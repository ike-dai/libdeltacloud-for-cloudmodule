#ifndef DCLOUD_API_H
#define DCLOUD_API_H

#include "common.h"
#include "geturl.h"
#include "link.h"
#include "instance.h"
#include "realm.h"
#include "flavor.h"
#include "image.h"
#include "instance_state.h"
#include "storage_volume.h"
#include "storage_snapshot.h"

struct deltacloud_api {
  char *url;
  char *user;
  char *password;

  struct link *links;
};

int get_links(struct deltacloud_api *api);

int get_instances(struct deltacloud_api *api, struct instance **instances);
int get_instance_by_id(struct deltacloud_api *api, const char *id,
		       struct instance *instance);

int get_realms(struct deltacloud_api *api, struct realm **realms);
int get_realm_by_id(struct deltacloud_api *api, const char *id,
		    struct realm *realm);

int get_flavors(struct deltacloud_api *api, struct flavor **flavors);
int get_flavor_by_id(struct deltacloud_api *api, const char *id,
		     struct flavor *flavor);
int get_flavor_by_uri(struct deltacloud_api *api, const char *url,
		      struct flavor *flavor);

int get_images(struct deltacloud_api *api, struct image **images);
int get_image_by_id(struct deltacloud_api *api, const char *id,
		    struct image *image);

int get_instance_states(struct deltacloud_api *api,
			struct instance_state **instance_states);
int get_instance_state_by_name(struct deltacloud_api *api, const char *name,
			       struct instance_state *instance_state);

int get_storage_volumes(struct deltacloud_api *api,
			struct storage_volume **storage_volumes);
int get_storage_volume_by_id(struct deltacloud_api *api, const char *id,
			     struct storage_volume *storage_volume);

int get_storage_snapshots(struct deltacloud_api *api,
			  struct storage_snapshot **storage_snapshots);
int get_storage_snapshot_by_id(struct deltacloud_api *api, const char *id,
			       struct storage_snapshot *storage_snapshot);

struct instance *create_instance(struct deltacloud_api *api,
				 const char *image_id, const char *name,
				 const char *realm_id, const char *flavor_id);

#endif