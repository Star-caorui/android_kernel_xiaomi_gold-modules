// SPDX-License-Identifier: GPL-2.0
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/sched/rt.h>
#include <linux/string.h>
#include <uapi/linux/sched/types.h>
#include <uapi/linux/android/binder.h>
#include <trace/hooks/binder.h>
#include "binder_internal.h"

#define CREATE_TRACE_POINTS
#include "binder_prio.h"

struct binder_prio_task_entry {
	const char *group_leader_comm;
	const char *thread_comm;
	const char *target_proc_comm;
};

static const struct binder_prio_task_entry lunch_name_tid[5] = {
	{ "surfaceflinger", "wmshell.main", "system_server" },
	{ "com.miui.home",  "ll.splashscreen", "system_server" },
	{ "android.systemui", "wmshell.main", "system_server" },
	{ "surfaceflinger", "wmshell.main", "com.miui.home" },
	{ "com.miui.home",  "ll.splashscreen", "com.miui.home" },
};

static void extend_surfacefinger_binder_set_priority_handler(void *data,
	struct binder_transaction *t, struct task_struct *task)
{
	struct sched_param params;
	struct binder_priority desired;
	unsigned int policy;
	struct binder_node *target_node = t->buffer->target_node;

	desired.prio = target_node->min_priority;
	desired.sched_policy = target_node->sched_policy;

	if (!t || !t->from)
		goto out_set;

	if (t->from->task && !(t->flags & TF_ONE_WAY) &&
	    rt_policy(t->from->task->policy) &&
	    t->from->task->pid == t->from->task->tgid) {
		if (!strncmp(t->from->task->comm, "com.miui.home", strlen("com.miui.home")) ||
		    !strncmp(t->from->task->comm, "android.systemui", strlen("android.systemui")) ||
		    !strncmp(t->from->task->comm, "surfaceflinger", strlen("surfaceflinger"))) {
			desired.prio = 98;
			desired.sched_policy = SCHED_FIFO;
			goto apply_prio;
		}
	}

	if (t->from->task && !(t->flags & TF_ONE_WAY) &&
	    rt_policy(t->from->task->policy)) {
		if (!strncmp(t->from->task->comm, "wmshell.main", strlen("wmshell.main")) ||
		    !strncmp(t->from->task->comm, "ll.splashscreen", strlen("ll.splashscreen"))) {
			desired.prio = 98;
			desired.sched_policy = SCHED_FIFO;
			goto apply_prio;
		}
	}

out_set:
	policy = desired.sched_policy;
	if (!rt_policy(policy))
		return;

apply_prio:
	policy = desired.sched_policy;
	if (task->policy != policy) {
		params.sched_priority = MAX_RT_PRIO - 1 - desired.prio;
		sched_setscheduler_nocheck(task, policy | SCHED_RESET_ON_FORK, &params);
	}
}

static void extend_surfacefinger_binder_trans_handler(void *data,
	struct binder_proc *target_proc, struct binder_proc *proc,
	struct binder_thread *thread, struct binder_transaction_data *tr)
{
	if (!target_proc || !target_proc->tsk)
		return;

	if (!strncmp(target_proc->tsk->comm, "surfaceflinger", strlen("surfaceflinger"))) {
		if (proc && thread && tr && thread->transaction_stack &&
		    !(thread->transaction_stack->flags & TF_ONE_WAY)) {
			target_proc->default_priority.sched_policy = SCHED_FIFO;
			target_proc->default_priority.prio = 98;
		}
	}
}

static void extend_android_vh_binder_proc_transaction_finish(void *data,
	struct binder_proc *proc, struct binder_transaction *t,
	struct task_struct *binder_th_task, bool pending_async, bool sync)
{
	struct sched_param params = { 0 };
	unsigned int cur_policy = current->policy;
	int i;

	if (sync || !rt_policy(cur_policy))
		return;

	if (!binder_th_task || !proc || !current->group_leader || !proc->tsk)
		return;

	for (i = 0; i < ARRAY_SIZE(lunch_name_tid); i++) {
		if (strcmp(lunch_name_tid[i].group_leader_comm, current->group_leader->comm))
			continue;

		if (strcmp(lunch_name_tid[i].thread_comm, current->comm) &&
		    !strstr(current->comm, lunch_name_tid[i].thread_comm))
			continue;

		if (!strcmp(lunch_name_tid[i].target_proc_comm, proc->tsk->comm)) {
			params.sched_priority = MAX_RT_PRIO - 1 - current->prio;
			if (cur_policy != binder_th_task->policy ||
			    params.sched_priority != binder_th_task->prio) {
				trace_binder_prio_proc_transaction_finish(binder_th_task->comm);
				sched_setscheduler_nocheck(binder_th_task,
					SCHED_FIFO | SCHED_RESET_ON_FORK, &params);
			}
		}
	}
}

static int __init binder_prio_init(void)
{
	pr_info("binder_prio: module init!");
	register_trace_android_vh_binder_set_priority(
		extend_surfacefinger_binder_set_priority_handler, NULL);
	register_trace_android_vh_binder_trans(
		extend_surfacefinger_binder_trans_handler, NULL);
	register_trace_android_vh_binder_proc_transaction_finish(
		extend_android_vh_binder_proc_transaction_finish, NULL);
	return 0;
}

static void __exit binder_prio_exit(void)
{
	unregister_trace_android_vh_binder_set_priority(
		extend_surfacefinger_binder_set_priority_handler, NULL);
	unregister_trace_android_vh_binder_trans(
		extend_surfacefinger_binder_trans_handler, NULL);
	unregister_trace_android_vh_binder_proc_transaction_finish(
		extend_android_vh_binder_proc_transaction_finish, NULL);
	pr_info("binder_prio: module exit!");
}

module_init(binder_prio_init);
module_exit(binder_prio_exit);
MODULE_LICENSE("GPL");
