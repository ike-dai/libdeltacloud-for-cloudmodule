#ifndef DCLOUD_API_H
#define DCLOUD_API_H

#include "link.h"
#include "instance.h"
#include "realm.h"
#include "image.h"
#include "instance_state.h"
#include "storage_volume.h"
#include "storage_snapshot.h"
#include "hardware_profile.h"

#ifdef __cplusplus
extern "C" {
#endif

struct deltacloud_api {
  char *url;
  char *user;
  char *password;

  struct deltacloud_link *links;
};

struct deltacloud_error {
  int error_num;
  char *details;
};

int deltacloud_initialize(struct deltacloud_api *api, char *url, char *user,
			  char *password);

int deltacloud_get_instances(struct deltacloud_api *api,
			     struct deltacloud_instance **instances);
int deltacloud_get_instance_by_id(struct deltacloud_api *api, const char *id,
				  struct deltacloud_instance *instance);
int deltacloud_get_instance_by_name(struct deltacloud_api *api,
				    const char *name,
				    struct deltacloud_instance *instance);

int deltacloud_get_realms(struct deltacloud_api *api,
			  struct deltacloud_realm **realms);
int deltacloud_get_realm_by_id(struct deltacloud_api *api, const char *id,
			       struct deltacloud_realm *realm);

int deltacloud_get_images(struct deltacloud_api *api,
			  struct deltacloud_image **images);
int deltacloud_get_image_by_id(struct deltacloud_api *api, const char *id,
			       struct deltacloud_image *image);

int deltacloud_get_instance_states(struct deltacloud_api *api,
				   struct deltacloud_instance_state **instance_states);
int deltacloud_get_instance_state_by_name(struct deltacloud_api *api,
					  const char *name,
					  struct deltacloud_instance_state *instance_state);

int deltacloud_get_storage_volumes(struct deltacloud_api *api,
				   struct deltacloud_storage_volume **storage_volumes);
int deltacloud_get_storage_volume_by_id(struct deltacloud_api *api,
					const char *id,
					struct deltacloud_storage_volume *storage_volume);

int deltacloud_get_storage_snapshots(struct deltacloud_api *api,
				     struct deltacloud_storage_snapshot **storage_snapshots);
int deltacloud_get_storage_snapshot_by_id(struct deltacloud_api *api,
					  const char *id,
					  struct deltacloud_storage_snapshot *storage_snapshot);

int deltacloud_get_hardware_profiles(struct deltacloud_api *api,
				     struct deltacloud_hardware_profile **hardware_profiles);
int deltacloud_get_hardware_profile_by_id(struct deltacloud_api *api,
					  const char *id,
					  struct deltacloud_hardware_profile *profile);

int deltacloud_create_instance(struct deltacloud_api *api, const char *image_id,
			       const char *name, const char *realm_id,
			       const char *hardware_profile,
			       const char *keyname,
			       const char *user_data,
			       struct deltacloud_instance *inst);
int deltacloud_instance_stop(struct deltacloud_api *api,
			     struct deltacloud_instance *instance);
int deltacloud_instance_reboot(struct deltacloud_api *api,
			       struct deltacloud_instance *instance);
int deltacloud_instance_start(struct deltacloud_api *api,
			      struct deltacloud_instance *instance);
int deltacloud_instance_destroy(struct deltacloud_api *api,
				struct deltacloud_instance *instance);

struct deltacloud_error *deltacloud_get_last_error(void);
const char *deltacloud_get_last_error_string(void);

void deltacloud_free(struct deltacloud_api *api);

/* Error codes */
#define DELTACLOUD_UNKNOWN_ERROR -1
/* ERROR codes -2, -3, and -4 are reserved for future use */
#define DELTACLOUD_GET_URL_ERROR -5
#define DELTACLOUD_POST_URL_ERROR -6
#define DELTACLOUD_XML_ERROR -7
#define DELTACLOUD_URL_DOES_NOT_EXIST_ERROR -8
#define DELTACLOUD_OOM_ERROR -9
#define DELTACLOUD_INVALID_ARGUMENT_ERROR -10
#define DELTACLOUD_NAME_NOT_FOUND_ERROR -11
#define DELTACLOUD_DELETE_URL_ERROR -12

#ifdef __cplusplus
}
#endif

#endif
