/*
 * Copyright (C) 2011 Red Hat, Inc.
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "libdeltacloud.h"
#include "test_common.h"

static void print_realm(struct deltacloud_realm *realm)
{
  fprintf(stderr, "Realm: %s\n", realm->name);
  fprintf(stderr, "\tHref: %s\n", realm->href);
  fprintf(stderr, "\tID: %s\n", realm->id);
  fprintf(stderr, "\tLimit: %s\n", realm->limit);
  fprintf(stderr, "\tState: %s\n", realm->state);
}

static void print_realm_list(struct deltacloud_realm *realms)
{
  struct deltacloud_realm *realm;

  deltacloud_for_each(realm, realms)
    print_realm(realm);
}

int main(int argc, char *argv[])
{
  struct deltacloud_api api;
  struct deltacloud_api zeroapi;
  struct deltacloud_realm realm;
  struct deltacloud_realm *realms;
  int ret = 3;

  if (argc != 4) {
    fprintf(stderr, "Usage: %s <url> <user> <password>\n", argv[0]);
    return 1;
  }

  if (deltacloud_initialize(&api, argv[1], argv[2], argv[3]) < 0) {
    fprintf(stderr, "Failed to find links for the API: %s\n",
	    deltacloud_get_last_error_string());
    return 2;
  }

  memset(&zeroapi, 0, sizeof(struct deltacloud_api));

  /* test out deltacloud_supports_realms */
  if (deltacloud_supports_realms(NULL) >= 0) {
    fprintf(stderr, "Expected deltacloud_supports_realms to fail with NULL api, but succeeded\n");
    goto cleanup;
  }

  if (deltacloud_supports_realms(&zeroapi) >= 0) {
    fprintf(stderr, "Expected deltacloud_supports_realms to fail with uninitialized api, but succeeded\n");
    goto cleanup;
  }

  if (deltacloud_supports_realms(&api)) {

    /* test out deltacloud_get_realms */
    if (deltacloud_get_realms(NULL, &realms) >= 0) {
      fprintf(stderr, "Expected deltacloud_get_realms to fail with NULL api, but succeeded\n");
      goto cleanup;
    }

    if (deltacloud_get_realms(&api, NULL) >= 0) {
      fprintf(stderr, "Expected deltacloud_get_realms to fail with NULL realms, but succeeded\n");
      goto cleanup;
    }

    if (deltacloud_get_realms(&zeroapi, &realms) >= 0) {
      fprintf(stderr, "Expected deltacloud_get_realms to fail with unintialized api, but succeeded\n");
      goto cleanup;
    }

    if (deltacloud_get_realms(&api, &realms) < 0) {
      fprintf(stderr, "Failed to get_realms: %s\n",
	      deltacloud_get_last_error_string());
      goto cleanup;
    }
    print_realm_list(realms);

    if (realms != NULL) {

      /* test out deltacloud_get_realm_by_id */
      if (deltacloud_get_realm_by_id(NULL, realms->id, &realm) >= 0) {
	fprintf(stderr, "Expected deltacloud_get_realm_by_id to fail with NULL api, but succeeded\n");
	goto cleanup;
      }

      if (deltacloud_get_realm_by_id(&api, NULL, &realm) >= 0) {
	fprintf(stderr, "Expected deltacloud_get_realm_by_id to fail with NULL id, but succeeded\n");
	goto cleanup;
      }

      if (deltacloud_get_realm_by_id(&api, realms->id, NULL) >= 0) {
	fprintf(stderr, "Expected deltacloud_get_realm_by_id to fail with NULL realm, but succeeded\n");
	goto cleanup;
      }

      if (deltacloud_get_realm_by_id(&api, "bogus_id", &realm) >= 0) {
	fprintf(stderr, "Expected deltacloud_get_realm_by_id to fail with bogus id, but succeeded\n");
	goto cleanup;
      }

      if (deltacloud_get_realm_by_id(&zeroapi, realms->id, &realm) >= 0) {
	fprintf(stderr, "Expected deltacloud_get_realm_by_id to fail with unintialized api, but succeeded\n");
	goto cleanup;
      }

      /* here we use the first realm from the list above */
      if (deltacloud_get_realm_by_id(&api, realms->id, &realm) < 0) {
	fprintf(stderr, "Failed to get realm by id: %s\n",
		deltacloud_get_last_error_string());
	goto cleanup;
      }
      print_realm(&realm);
      deltacloud_free_realm(&realm);
    }
  }
  else
    fprintf(stderr, "Realms are not supported\n");

  ret = 0;

 cleanup:
  deltacloud_free_realm_list(&realms);

  deltacloud_free(&api);

  return ret;
}
