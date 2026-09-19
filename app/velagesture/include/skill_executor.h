/****************************************************************************
 * app/velagesture/include/skill_executor.h
 *
 * Skill executor — loads and invokes agent skills.
 *
 * In the VelaGesture architecture, the primary skill is
 * "guxian-control" which exposes query_status and execute_action
 * capabilities against the Guxian business system.
 *
 * Skills are defined as SKILL.md files under /data/agent/skills/.
 * The executor parses the skill definition and dispatches calls.
 *
 * SPDX-License-Identifier: Apache-2.0
 ****************************************************************************/

#ifndef APP_VELAGESTURE_INCLUDE_SKILL_EXECUTOR_H
#define APP_VELAGESTURE_INCLUDE_SKILL_EXECUTOR_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "velagesture.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define VG_SKILL_PATH_PREFIX  "/data/agent/skills"
#define VG_SKILL_NAME_GUXIAN  "guxian-control"

/****************************************************************************
 * Public Types
 ****************************************************************************/

/* Skill call — describes one invocation of a skill action. */

struct vg_skill_call_s
{
  const char *skill_name;     /* e.g. "guxian-control" */
  const char *action;         /* e.g. "query_status" or "execute_action" */
  const char *params_json;    /* action-specific parameters as JSON */
};

/* Skill result — returned by the executor after a call completes. */

struct vg_skill_result_s
{
  int   code;                 /* 0 = success, negative = error */
  char  payload[VG_MAX_PAYLOAD_LEN]; /* result data as JSON */
};

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

/****************************************************************************
 * Name: vg_skill_init
 *
 * Description:
 *   Initialise the skill executor.  Scans /data/agent/skills/ for
 *   available skills and builds an internal registry.
 *
 ****************************************************************************/

int vg_skill_init(void);

/****************************************************************************
 * Name: vg_skill_call
 *
 * Description:
 *   Execute a skill call synchronously.  The executor looks up the
 *   named skill, invokes the requested action with the given params,
 *   and populates *result.
 *
 ****************************************************************************/

int vg_skill_call(const struct vg_skill_call_s *call,
                  struct vg_skill_result_s *result);

/****************************************************************************
 * Name: vg_skill_query_status
 *
 * Description:
 *   Convenience wrapper: invoke guxian-control/query_status.
 *
 ****************************************************************************/

int vg_skill_query_status(const char *target, struct vg_skill_result_s *result);

/****************************************************************************
 * Name: vg_skill_execute_action
 *
 * Description:
 *   Convenience wrapper: invoke guxian-control/execute_action.
 *
 ****************************************************************************/

int vg_skill_execute_action(const char *action_json,
                            struct vg_skill_result_s *result);

/****************************************************************************
 * Name: vg_skill_shutdown
 *
 * Description:
 *   Release skill executor resources.
 *
 ****************************************************************************/

void vg_skill_shutdown(void);

#endif /* APP_VELAGESTURE_INCLUDE_SKILL_EXECUTOR_H */
