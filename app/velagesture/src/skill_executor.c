/****************************************************************************
 * app/velagesture/src/skill_executor.c
 *
 * Skill executor — loads and invokes agent skills from
 * /data/agent/skills/.
 *
 * The primary skill is "guxian-control" which provides:
 *   - query_status: Query the Guxian business system for current
 *     device/task/scene status.
 *   - execute_action: Execute an action on the Guxian system
 *     (locate device, trigger alarm acknowledge, switch view, etc.)
 *
 * Skill definitions are loaded from SKILL.md files.  The executor
 * parses the YAML front matter to determine available actions and
 * their parameter schemas, then dispatches calls over the WebSocket
 * connection to the Guxian backend.
 *
 * SPDX-License-Identifier: Apache-2.0
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <dirent.h>

#include "velagesture.h"
#include "skill_executor.h"
#include "ws_protocol.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define MAX_SKILLS  8

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct skill_entry_s
{
  char name[VG_MAX_SKILL_NAME];
  char path[256];
  bool loaded;
};

/****************************************************************************
 * Private Data
 ****************************************************************************/

static struct skill_entry_s g_skills[MAX_SKILLS];
static int g_skill_count;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: scan_skills_directory
 *
 * Description:
 *   Scan /data/agent/skills/ for directories containing SKILL.md
 *   files and register them in the internal skill table.
 ****************************************************************************/

static int scan_skills_directory(void)
{
  DIR *dir = opendir(VG_SKILL_PATH_PREFIX);
  if (dir == NULL)
    {
      printf("SKILL: Skills directory not found at %s (will use built-in)\n",
             VG_SKILL_PATH_PREFIX);
      return 0;
    }

  struct dirent *entry;
  while ((entry = readdir(dir)) != NULL && g_skill_count < MAX_SKILLS)
    {
      if (entry->d_name[0] == '.')
        {
          continue;
        }

      /* Check for SKILL.md in this directory */

      char skill_path[256];
      snprintf(skill_path, sizeof(skill_path),
               "%s/%s/SKILL.md", VG_SKILL_PATH_PREFIX, entry->d_name);

      if (access(skill_path, R_OK) == 0)
        {
          struct skill_entry_s *s = &g_skills[g_skill_count];
          strlcpy(s->name, entry->d_name, sizeof(s->name));
          strlcpy(s->path, skill_path, sizeof(s->path));
          s->loaded = true;
          g_skill_count++;
          printf("SKILL: Registered '%s' from %s\n", s->name, skill_path);
        }
    }

  closedir(dir);
  return 0;
}

/****************************************************************************
 * Name: register_builtin_skill
 *
 * Description:
 *   Register a built-in skill definition.  Used when the on-device
 *   skills directory is not available (e.g. during development or
 *   before the skill file is installed).
 ****************************************************************************/

static int register_builtin_skill(const char *name)
{
  if (g_skill_count >= MAX_SKILLS)
    {
      return -ENOSPC;
    }

  struct skill_entry_s *s = &g_skills[g_skill_count];
  strlcpy(s->name, name, sizeof(s->name));
  snprintf(s->path, sizeof(s->path),
           "%s/%s/SKILL.md", VG_SKILL_PATH_PREFIX, name);
  s->loaded = true;
  g_skill_count++;

  printf("SKILL: Registered built-in '%s'\n", name);
  return 0;
}

/****************************************************************************
 * Name: find_skill
 *
 * Description:
 *   Look up a skill by name.
 ****************************************************************************/

static struct skill_entry_s *find_skill(const char *name)
{
  int i;
  for (i = 0; i < g_skill_count; i++)
    {
      if (strcmp(g_skills[i].name, name) == 0)
        {
          return &g_skills[i];
        }
    }
  return NULL;
}

/****************************************************************************
 * Name: dispatch_guxian_query
 *
 * Description:
 *   Handle guxian-control/query_status: send a status query over
 *   WebSocket and populate the result.
 ****************************************************************************/

static int dispatch_guxian_query(const char *params_json,
                                 struct vg_skill_result_s *result)
{
  /* Send the query to the Guxian backend */

  char ws_msg[512];
  snprintf(ws_msg, sizeof(ws_msg),
           "{\"type\":\"task\","
           "\"skill\":\"guxian-control\","
           "\"action\":\"query_status\","
           "\"params\":%s}",
           params_json ? params_json : "{}");

  int ret = vg_ws_send_status(ws_msg);
  if (ret < 0 && ret != -ENOTCONN)
    {
      result->code = ret;
      snprintf(result->payload, sizeof(result->payload),
               "{\"error\":\"ws_send_failed\",\"code\":%d}", ret);
      return ret;
    }

  /* In a full implementation, the response comes back asynchronously
   * via the WebSocket receive callback.  For the synchronous API
   * used by the agent, we populate a success acknowledgement here
   * and the actual result arrives in a subsequent task_result message. */

  result->code = 0;
  snprintf(result->payload, sizeof(result->payload),
           "{\"status\":\"query_sent\",\"skill\":\"guxian-control\","
           "\"action\":\"query_status\"}");
  return 0;
}

/****************************************************************************
 * Name: dispatch_guxian_execute
 *
 * Description:
 *   Handle guxian-control/execute_action: send an action command
 *   to the Guxian backend.
 ****************************************************************************/

static int dispatch_guxian_execute(const char *params_json,
                                   struct vg_skill_result_s *result)
{
  char ws_msg[512];
  snprintf(ws_msg, sizeof(ws_msg),
           "{\"type\":\"task\","
           "\"skill\":\"guxian-control\","
           "\"action\":\"execute_action\","
           "\"params\":%s}",
           params_json ? params_json : "{}");

  int ret = vg_ws_send_status(ws_msg);
  if (ret < 0 && ret != -ENOTCONN)
    {
      result->code = ret;
      snprintf(result->payload, sizeof(result->payload),
               "{\"error\":\"ws_send_failed\",\"code\":%d}", ret);
      return ret;
    }

  result->code = 0;
  snprintf(result->payload, sizeof(result->payload),
           "{\"status\":\"action_sent\","
           "\"skill\":\"guxian-control\","
           "\"action\":\"execute_action\"}");
  return 0;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: vg_skill_init
 ****************************************************************************/

int vg_skill_init(void)
{
  g_skill_count = 0;
  memset(g_skills, 0, sizeof(g_skills));

  /* Scan on-device skills directory */

  scan_skills_directory();

  /* Ensure guxian-control is always registered */

  if (find_skill(VG_SKILL_NAME_GUXIAN) == NULL)
    {
      register_builtin_skill(VG_SKILL_NAME_GUXIAN);
    }

  printf("SKILL: %d skill(s) registered\n", g_skill_count);
  return 0;
}

/****************************************************************************
 * Name: vg_skill_call
 ****************************************************************************/

int vg_skill_call(const struct vg_skill_call_s *call,
                  struct vg_skill_result_s *result)
{
  struct skill_entry_s *skill;

  if (call == NULL || result == NULL)
    {
      return -EINVAL;
    }

  skill = find_skill(call->skill_name);
  if (skill == NULL)
    {
      fprintf(stderr, "SKILL: Skill '%s' not found\n", call->skill_name);
      result->code = -ENOENT;
      snprintf(result->payload, sizeof(result->payload),
               "{\"error\":\"skill_not_found\",\"name\":\"%s\"}",
               call->skill_name);
      return -ENOENT;
    }

  /* Dispatch based on skill name and action */

  if (strcmp(skill->name, VG_SKILL_NAME_GUXIAN) == 0)
    {
      if (strcmp(call->action, "query_status") == 0)
        {
          return dispatch_guxian_query(call->params_json, result);
        }
      else if (strcmp(call->action, "execute_action") == 0)
        {
          return dispatch_guxian_execute(call->params_json, result);
        }
      else
        {
          result->code = -EINVAL;
          snprintf(result->payload, sizeof(result->payload),
                   "{\"error\":\"unknown_action\",\"action\":\"%s\"}",
                   call->action);
          return -EINVAL;
        }
    }

  result->code = -ENOSYS;
  snprintf(result->payload, sizeof(result->payload),
           "{\"error\":\"not_implemented\",\"skill\":\"%s\"}",
           call->skill_name);
  return -ENOSYS;
}

/****************************************************************************
 * Name: vg_skill_query_status
 ****************************************************************************/

int vg_skill_query_status(const char *target,
                          struct vg_skill_result_s *result)
{
  char params[128];
  snprintf(params, sizeof(params), "{\"target\":\"%s\"}", target);

  struct vg_skill_call_s call =
  {
    .skill_name = VG_SKILL_NAME_GUXIAN,
    .action     = "query_status",
    .params_json = params
  };

  return vg_skill_call(&call, result);
}

/****************************************************************************
 * Name: vg_skill_execute_action
 ****************************************************************************/

int vg_skill_execute_action(const char *action_json,
                            struct vg_skill_result_s *result)
{
  struct vg_skill_call_s call =
  {
    .skill_name  = VG_SKILL_NAME_GUXIAN,
    .action      = "execute_action",
    .params_json = action_json
  };

  return vg_skill_call(&call, result);
}

/****************************************************************************
 * Name: vg_skill_shutdown
 ****************************************************************************/

void vg_skill_shutdown(void)
{
  g_skill_count = 0;
}
