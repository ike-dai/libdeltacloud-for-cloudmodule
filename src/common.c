/*
 * Copyright (C) 2010,2011 Red Hat, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA
 *
 * Author: Chris Lalancette <clalance@redhat.com>
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdarg.h>
#include <pthread.h>
#include "libdeltacloud.h"
#include "common.h"
#include "curl_action.h"

int internal_destroy(const char *href, const char *user, const char *password)
{
  char *data = NULL;
  int ret = -1;

  /* in deltacloud the destroy action is a DELETE method, so we need
   * to use a different implementation
   */
  data = delete_url(href, user, password);
  if (data == NULL)
    /* delete_url sets its own errors, so don't overwrite it here */
    goto cleanup;

  if (is_error_xml(data)) {
    set_xml_error(data, DELTACLOUD_DELETE_URL_ERROR);
    goto cleanup;
  }
  ret = 0;

 cleanup:
  SAFE_FREE(data);

  return ret;
}

void free_parameters(struct deltacloud_create_parameter *params,
		     int params_length)
{
  int i;

  for (i = 0; i < params_length; i++)
    deltacloud_free_parameter_value(&params[i]);
}

int copy_parameters(struct deltacloud_create_parameter *dst,
		    struct deltacloud_create_parameter *src,
		    int params_length)
{
  int i;

  for (i = 0; i < params_length; i++) {
    if (deltacloud_prepare_parameter(&dst[i], src[i].name, src[i].value) < 0)
      /* this isn't entirely orthogonal, since this function can allocate
       * memory that it doesn't free.  We just depend on the higher layers
       * to free the whole thing as necessary
       */
      /* deltacloud_prepare_parameter already set the error */
      return -1;
  }

  return i;
}

static int add_safe_value(FILE *fp, const char *name, const char *value)
{
  char *safevalue;

  safevalue = curl_escape(value, 0);
  if (safevalue == NULL) {
    oom_error();
    return -1;
  }

  /* if we are not at the beginning of the stream, we need to append a & */
  if (ftell(fp) != 0)
    fprintf(fp, "&");

  fprintf(fp, "%s=%s", name, safevalue);

  SAFE_FREE(safevalue);

  return 0;
}

int internal_create(struct deltacloud_api *api, const char *link,
		    struct deltacloud_create_parameter *params,
		    int params_length, char **headers)
{
  struct deltacloud_link *thislink;
  size_t param_string_length;
  FILE *paramfp;
  int ret = -1;
  char *data = NULL;
  char *param_string = NULL;
  int i;

  deltacloud_for_each(thislink, api->links) {
    if (STREQ(thislink->rel, link))
      break;
  }
  if (thislink == NULL) {
    link_error(link);
    return -1;
  }

  paramfp = open_memstream(&param_string, &param_string_length);
  if (paramfp == NULL) {
    oom_error();
    return -1;
  }

  /* since the parameters come from the user, we must not trust them and
   * URL escape them before use
   */

  for (i = 0; i < params_length; i++) {
    if (params[i].value != NULL) {
      if (add_safe_value(paramfp, params[i].name, params[i].value) < 0)
	/* add_safe_value already set the error */
	goto cleanup;
    }
  }

  fclose(paramfp);
  paramfp = NULL;

  if (post_url(thislink->href, api->user, api->password, param_string,
	       &data, headers) != 0)
    /* post_url sets its own errors, so don't overwrite it here */
    goto cleanup;

  if (data != NULL && is_error_xml(data)) {
    set_xml_error(data, DELTACLOUD_POST_URL_ERROR);
    goto cleanup;
  }

  ret = 0;

 cleanup:
  if (paramfp != NULL)
    fclose(paramfp);
  SAFE_FREE(param_string);
  SAFE_FREE(data);

  return ret;
}

static int parse_error_xml(xmlNodePtr cur, xmlXPathContextPtr ctxt, void **data)
{
  char **msg = (char **)data;

  *msg = getXPathString("string(/error/message)", ctxt);

  if (*msg == NULL)
    *msg = strdup("Unknown error");

  return 0;
}

void invalid_argument_error(const char *details)
{
  set_error(DELTACLOUD_INVALID_ARGUMENT_ERROR, details);
}

void set_xml_error(const char *xml, int type)
{
  char *errmsg = NULL;

  if (parse_xml(xml, "error", (void **)&errmsg, parse_error_xml, 1) < 0)
    errmsg = strdup("Unknown error");

  set_error(type, errmsg);

  SAFE_FREE(errmsg);
}

int is_error_xml(const char *xml)
{
  return STRPREFIX(xml, "<error");
}

void link_error(const char *name)
{
  char *tmp;
  int alloc_fail = 0;

  if (asprintf(&tmp, "Failed to find the link for '%s'", name) < 0) {
    tmp = "Failed to find the link";
    alloc_fail = 1;
  }

  set_error(DELTACLOUD_URL_DOES_NOT_EXIST_ERROR, tmp);
  if (!alloc_fail)
    SAFE_FREE(tmp);
}

static void data_error(const char *name)
{
  char *tmp;
  int alloc_fail = 0;

  if (asprintf(&tmp, "Expected %s data, received nothing", name) < 0) {
    tmp = "Expected data, received nothing";
    alloc_fail = 1;
  }

  set_error(DELTACLOUD_GET_URL_ERROR, tmp);
  if (!alloc_fail)
    SAFE_FREE(tmp);
}

/*
 * An internal function for fetching all of the elements of a particular
 * type.  Note that although relname and rootname is the same for almost
 * all types, there are a couple of them that don't conform to this pattern.
 */
int internal_get(struct deltacloud_api *api, const char *relname,
		 const char *rootname,
		 int (*xml_cb)(xmlNodePtr, xmlXPathContextPtr, void **),
		 void **output)
{
  struct deltacloud_link *thislink = NULL;
  char *data = NULL;
  int ret = -1;

  /* we only check api and output here, as those are the only parameters from
   * the user
   */
  if (!valid_arg(api) || !valid_arg(output))
    return -1;

  deltacloud_for_each(thislink, api->links) {
    if (STREQ(thislink->rel, relname))
      break;
  }
  if (thislink == NULL) {
    link_error(relname);
    return -1;
  }

  if (get_url(thislink->href, api->user, api->password, &data) != 0)
    /* get_url sets its own errors, so don't overwrite it here */
    return -1;

  if (data == NULL) {
    /* if we made it here, it means that the transfer was successful (ret
     * was 0), but the data that we expected wasn't returned.  This is probably
     * a deltacloud server bug, so just set an error and bail out
     */
    data_error(relname);
    goto cleanup;
  }

  if (is_error_xml(data)) {
    set_xml_error(data, DELTACLOUD_GET_URL_ERROR);
    goto cleanup;
  }

  *output = NULL;
  if (parse_xml(data, rootname, output, xml_cb, 1) < 0)
    goto cleanup;

  ret = 0;

 cleanup:
  SAFE_FREE(data);

  return ret;
}

static int parse_xml_single(const char *xml_string, const char *name,
			    int (*cb)(xmlNodePtr cur, xmlXPathContextPtr ctxt,
				      void *data),
			    void *output)
{
  xmlDocPtr xml;
  xmlNodePtr root;
  xmlXPathContextPtr ctxt = NULL;
  int ret = -1;

  xml = xmlReadDoc(BAD_CAST xml_string, name, NULL,
		   XML_PARSE_NOENT | XML_PARSE_NONET | XML_PARSE_NOERROR |
		   XML_PARSE_NOWARNING);
  if (!xml) {
    set_error_from_xml(name, "Failed to parse XML");
    return -1;
  }

  root = xmlDocGetRootElement(xml);
  if (root == NULL) {
    set_error_from_xml(name, "Failed to get the root element");
    goto cleanup;
  }

  if (STRNEQ((const char *)root->name, name)) {
    xml_error(name, "Failed to get expected root element", (char *)root->name);
    goto cleanup;
  }

  ctxt = xmlXPathNewContext(xml);
  if (ctxt == NULL) {
    set_error_from_xml(name, "Failed to initialize XPath context");
    goto cleanup;
  }

  ctxt->node = root;

  if (cb(root, ctxt, output) < 0)
    /* the callbacks are expected to have set their own error */
    goto cleanup;

  ret = 0;

 cleanup:
  if (ctxt != NULL)
    xmlXPathFreeContext(ctxt);
  xmlFreeDoc(xml);

  return ret;
}

int internal_get_by_id(struct deltacloud_api *api, const char *id,
		       const char *relname, const char *rootname,
		       int (*cb)(xmlNodePtr cur, xmlXPathContextPtr ctxt,
				 void *data),
		       void *output)
{
  char *url = NULL;
  char *data = NULL;
  char *safeid;
  int ret = -1;

  /* we only check api, id, and output here, as those are the only parameters
   * from the user
   */
  if (!valid_arg(api) || !valid_arg(id) || !valid_arg(output))
    return -1;

  safeid = curl_escape(id, 0);
  if (safeid == NULL) {
    oom_error();
    return -1;
  }

  if (asprintf(&url, "%s/%s/%s", api->url, relname, safeid) < 0) {
    oom_error();
    goto cleanup;
  }

  if (get_url(url, api->user, api->password, &data) != 0)
    /* get_url sets its own errors, so don't overwrite it here */
    goto cleanup;

  if (data == NULL) {
    /* if we made it here, it means that the transfer was successful (ret
     * was 0), but the data that we expected wasn't returned.  This is probably
     * a deltacloud server bug, so just set an error and bail out
     */
    data_error(relname);
    goto cleanup;
  }

  if (is_error_xml(data)) {
    set_xml_error(data, DELTACLOUD_GET_URL_ERROR);
    goto cleanup;
  }

  if (parse_xml_single(data, rootname, cb, output) < 0)
    goto cleanup;

  ret = 0;

 cleanup:
  SAFE_FREE(data);
  SAFE_FREE(url);
  curl_free(safeid);

  return ret;
}

char *getXPathString(const char *xpath, xmlXPathContextPtr ctxt)
{
  xmlXPathObjectPtr obj;
  xmlNodePtr relnode;
  char *ret;

  if ((ctxt == NULL) || (xpath == NULL))
    return NULL;

  relnode = ctxt->node;
  obj = xmlXPathEval(BAD_CAST xpath, ctxt);
  ctxt->node = relnode;
  if ((obj == NULL) || (obj->type != XPATH_STRING) ||
      (obj->stringval == NULL) || (obj->stringval[0] == 0)) {
    xmlXPathFreeObject(obj);
    return NULL;
  }
  ret = strdup((char *) obj->stringval);
  xmlXPathFreeObject(obj);

  return ret;
}

void oom_error(void)
{
  set_error(DELTACLOUD_OOM_ERROR, "Failed to allocate memory");
}

void xml_error(const char *name, const char *type, const char *details)
{
  char *tmp;
  int alloc_fail = 0;

  if (asprintf(&tmp, "%s for %s: %s", type, name, details) < 0) {
    tmp = "Failed parsing XML";
    alloc_fail = 1;
  }

  set_error(DELTACLOUD_XML_ERROR, tmp);
  if (!alloc_fail)
    SAFE_FREE(tmp);
}

void set_error_from_xml(const char *name, const char *usermsg)
{
  xmlErrorPtr last;
  char *msg;

  last = xmlGetLastError();
  if (last != NULL)
    msg = last->message;
  else
    msg = "unknown error";
  xml_error(name, usermsg, msg);
}

int parse_xml(const char *xml_string, const char *name, void **data,
	      int (*cb)(xmlNodePtr cur, xmlXPathContextPtr ctxt, void **data),
	      int multiple)
{
  xmlDocPtr xml;
  xmlNodePtr root;
  xmlXPathContextPtr ctxt = NULL;
  int ret = -1;
  int rc;

  xml = xmlReadDoc(BAD_CAST xml_string, name, NULL,
		   XML_PARSE_NOENT | XML_PARSE_NONET | XML_PARSE_NOERROR |
		   XML_PARSE_NOWARNING);
  if (!xml) {
    set_error_from_xml(name, "Failed to parse XML");
    return -1;
  }

  root = xmlDocGetRootElement(xml);
  if (root == NULL) {
    set_error_from_xml(name, "Failed to get the root element");
    goto cleanup;
  }

  if (STRNEQ((const char *)root->name, name)) {
    xml_error(name, "Failed to get expected root element", (char *)root->name);
    goto cleanup;
  }

  ctxt = xmlXPathNewContext(xml);
  if (ctxt == NULL) {
    set_error_from_xml(name, "Failed to initialize XPath context");
    goto cleanup;
  }

  /* if "multiple" is true, then the XML looks something like:
   * <instances> <instance> ... </instance> </instances>"
   * if "multiple" is false, then the XML looks something like:
   * <instance> ... </instance>
   */
  if (multiple)
    rc = cb(root->children, ctxt, data);
  else
    rc = cb(root, ctxt, data);

  if (rc < 0)
    /* the callbacks are expected to have set their own error */
    goto cleanup;

  ret = 0;

 cleanup:
  if (ctxt != NULL)
    xmlXPathFreeContext(ctxt);
  xmlFreeDoc(xml);

  return ret;
}

pthread_key_t deltacloud_last_error;

void free_and_null(void *ptrptr)
{
  free (*(void**)ptrptr);
  *(void**)ptrptr = NULL;
}

int strdup_or_null(char **out, const char *in)
{
  if (in == NULL) {
    *out = NULL;
    return 0;
  }

  *out = strdup(in);

  if (*out == NULL)
    return -1;

  return 0;
}

void dcloudprintf(const char *fmt, ...)
{
  va_list va_args;

  if (getenv("LIBDELTACLOUD_DEBUG")) {
    va_start(va_args, fmt);
    vfprintf(stderr, fmt, va_args);
    va_end(va_args);
  }
}

void deltacloud_error_free_data(void *data)
{
  struct deltacloud_error *err = data;

  if (err == NULL)
    return;

  SAFE_FREE(err->details);
  SAFE_FREE(err);
}

void set_error(int errnum, const char *details)
{
  struct deltacloud_error *err;
  struct deltacloud_error *last;

  err = (struct deltacloud_error *)malloc(sizeof(struct deltacloud_error));
  if (err == NULL) {
    /* if we failed to allocate memory here, there's not a lot we can do */
    dcloudprintf("Failed to allocate memory in an error path; error information will be unreliable!\n");
    return;
  }
  memset(err, 0, sizeof(struct deltacloud_error));

  err->error_num = errnum;
  err->details = strdup(details);

  dcloudprintf("%s\n", err->details);

  last = pthread_getspecific(deltacloud_last_error);
  if (last != NULL)
    deltacloud_error_free_data(last);

  pthread_setspecific(deltacloud_last_error, err);
}

#ifdef DEBUG

#ifndef FAILRATE
#define FAILRATE 1000
#endif

static void seed_random(void)
{
  static int done = 0;

  if (done)
    return;

  srand(time(NULL));
  done = 1;
}

#undef xmlReadDoc
xmlDocPtr xmlReadDoc_sometimes_fail(const xmlChar *cur,
				    const char *URL,
				    const char *encoding,
				    int options)
{
  seed_random();

  if (rand() % FAILRATE)
    return xmlReadDoc(cur, URL, encoding, options);
  else
    return NULL;
}

#undef xmlDocGetRootElement
xmlNodePtr xmlDocGetRootElement_sometimes_fail(xmlDocPtr doc)
{
  seed_random();

  if (rand() % FAILRATE)
    return xmlDocGetRootElement(doc);
  else
    return NULL;
}

#undef xmlXPathNewContext
xmlXPathContextPtr xmlXPathNewContext_sometimes_fail(xmlDocPtr doc)
{
  seed_random();

  if (rand() % FAILRATE)
    return xmlXPathNewContext(doc);
  else
    return NULL;
}

#undef xmlXPathEval
xmlXPathObjectPtr xmlXPathEval_sometimes_fail(const xmlChar *str,
					      xmlXPathContextPtr ctx)
{
  seed_random();

  if (rand() % FAILRATE)
    return xmlXPathEval(str, ctx);
  else
    return NULL;
}

#undef asprintf
int asprintf_sometimes_fail(char **strp, const char *fmt, ...)
{
  va_list ap;
  int ret;

  seed_random();

  if (rand() % FAILRATE) {
    va_start(ap, fmt);
    ret = vasprintf(strp, fmt, ap);
    va_end(ap);
    return ret;
  }
  else
    return -1;
}

#undef malloc
void *malloc_sometimes_fail(size_t size)
{
  seed_random();

  if (rand() % FAILRATE)
    return malloc(size);
  else
    return NULL;
}

#undef realloc
void *realloc_sometimes_fail(void *ptr, size_t size)
{
  seed_random();

  if (rand() % FAILRATE)
    return realloc(ptr, size);
  else
    return NULL;
}

#undef curl_easy_init
CURL *curl_easy_init_sometimes_fail(void)
{
  seed_random();

  if (rand() % FAILRATE)
    return curl_easy_init();
  else
    return NULL;
}

#undef curl_slist_append
struct curl_slist *curl_slist_append_sometimes_fail(struct curl_slist *list,
						    const char *string)
{
  seed_random();

  if (rand() % FAILRATE)
    return curl_slist_append(list, string);
  else
    return NULL;
}

#undef curl_easy_perform
CURLcode curl_easy_perform_sometimes_fail(CURL *handle)
{
  seed_random();

  if (rand() % FAILRATE)
    return curl_easy_perform(handle);
  else
    return CURLE_OUT_OF_MEMORY;
}

#undef strdup
char *strdup_sometimes_fail(const char *s)
{
  seed_random();

  if (rand() % FAILRATE)
    return strdup(s);
  else
    return NULL;
}

#endif /* DEBUG */