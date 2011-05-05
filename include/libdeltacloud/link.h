/*
 * Copyright (C) 2010 Red Hat, Inc.
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

#ifndef LIBDELTACLOUD_LINK_H
#define LIBDELTACLOUD_LINK_H

#ifdef __cplusplus
extern "C" {
#endif

struct deltacloud_feature {
  char *name;

  struct deltacloud_feature *next;
};

struct deltacloud_link {
  char *href;
  char *rel;
  struct deltacloud_feature *features;

  struct deltacloud_link *next;
};

int add_to_feature_list(struct deltacloud_feature **features,
			struct deltacloud_feature *feature);
void free_feature(struct deltacloud_feature *feature);
void free_feature_list(struct deltacloud_feature **features);

int add_to_link_list(struct deltacloud_link **links,
		     struct deltacloud_link *link);
int copy_link_list(struct deltacloud_link **dst, struct deltacloud_link **src);
void free_link(struct deltacloud_link *link);
void free_link_list(struct deltacloud_link **links);

#ifdef __cplusplus
}
#endif

#endif
