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

static void print_instance_state(struct deltacloud_instance_state *state)
{
  struct deltacloud_instance_state_transition *transition;

  fprintf(stderr, "Instance State: %s\n", state->name);
  deltacloud_for_each(transition, state->transitions)
    fprintf(stderr, "\tTransition Action: %s, To: %s, Auto: %s\n",
	    transition->action, transition->to, transition->automatically);
}

static void print_instance_state_list(struct deltacloud_instance_state *states)
{
  struct deltacloud_instance_state *state;

  deltacloud_for_each(state, states)
    print_instance_state(state);
}

int main(int argc, char *argv[])
{
  struct deltacloud_api api;
  struct deltacloud_api zeroapi;
  struct deltacloud_instance_state *instance_states;
  int ret = 3;

  if (argc != 4) {
    fprintf(stderr, "Usage: %s <url> <user> <password>\n", argv[0]);
    return 1;
  }

  memset(&zeroapi, 0, sizeof(struct deltacloud_api));

  if (deltacloud_initialize(&api, argv[1], argv[2], argv[3]) < 0) {
    fprintf(stderr, "Failed to find links for the API: %s\n",
	    deltacloud_get_last_error_string());
    return 2;
  }

  /* test out deltacloud_supports_instance_states */
  if (deltacloud_supports_instance_states(NULL) >= 0) {
    fprintf(stderr, "Expected deltacloud_supports_instance_states to fail with NULL api, but succeeded\n");
    goto cleanup;
  }

  if (deltacloud_supports_instance_states(&zeroapi) >= 0) {
    fprintf(stderr, "Expected deltacloud_supports_instance_states to fail with uninitialized api, but succeeded\n");
    goto cleanup;
  }

  if (deltacloud_supports_instance_states(&api)) {

    /* test out deltacloud_get_instance_states */
    if (deltacloud_get_instance_states(NULL, &instance_states) >= 0) {
      fprintf(stderr, "Expected deltacloud_get_instance_states to fail with NULL api, but succeeded\n");
      goto cleanup;
    }

    if (deltacloud_get_instance_states(&api, NULL) >= 0) {
      fprintf(stderr, "Expected deltacloud_get_instance_states to fail with NULL instance_states, but succeeded\n");
      goto cleanup;
    }

    if (deltacloud_get_instance_states(&zeroapi, &instance_states) >= 0) {
      fprintf(stderr, "Expected deltacloud_get_instance_states to fail with unintialized api, but succeeded\n");
      goto cleanup;
    }

    if (deltacloud_get_instance_states(&api, &instance_states) < 0) {
      fprintf(stderr, "Failed to get_instance_states: %s\n",
	      deltacloud_get_last_error_string());
      goto cleanup;
    }
    print_instance_state_list(instance_states);

    deltacloud_free_instance_state_list(&instance_states);
  }
  else
    fprintf(stderr, "Instance States are not supported\n");

  ret = 0;

 cleanup:
  deltacloud_free(&api);

  return ret;
}
