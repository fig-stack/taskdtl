#include <linux/cred.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/pid.h>
#include <linux/sched/task.h>
#include <linux/threads.h>
#include <linux/uidgid.h>
#include <linux/sched/signal.h>

MODULE_AUTHOR("Fig");
MODULE_DESCRIPTION("display PID details");
MODULE_LICENSE("Dual MIT/GPL");
MODULE_VERSION("0.2");

/* module parameters */
static int pid;
module_param(pid, int, 0660);
MODULE_PARM_DESC(pid, "select pid to analyze");

static long process_task(struct task_struct *task)
{
	const struct cred *cred = get_task_cred(task);
	unsigned int       uid;
	unsigned int       euid;

	if (cred) {
		euid = from_kuid(&init_user_ns, cred->euid);
		uid  = from_kuid(&init_user_ns, cred->uid);
	}

	get_task_struct(task);

	if (likely(in_task())) {
		/* general info*/
		pr_info(" name  : %s\n"
	        	" PID   : %6d\n"
		        " TPID  : %6d\n"
			        " UID   : %6d\n"
	        	" EUID  : %6u (%s root)\n"
	        	" state : %6c\n"
	       		" current (ptr to task_struct) :\n"
	        	"               0x%pK (0x%px)\n",
	        	task->comm, task_pid_nr(task), task_tgid_nr(task), uid, euid,
	        	(euid == 0 ? "have" : "don't have"), task_state_to_char(task), task, task);
	
		pr_info(" # threads    : %d\n"
			" stack        : 0x%px\n"
			" flags        : %u\n"
			" sched ::\n"
			" curr_cpu     : %d\n"
			" on RQ?       : %s\n"
			" prio         : %d\n"
			" static prio  : %d\n"
			" normal prio  : %d\n"
			" RT priority  : %u\n",
			get_nr_threads(task), task->stack, task->flags, 
			task->recent_used_cpu, (task->on_rq ? "yes" : "no"),
			task->prio, task->static_prio, task->normal_prio,
			task->rt_priority);
	} else {
		pr_alert("running in interrupt context [should not happen here!]\n");
	}

	put_task_struct(task);

	return 0;
}

static int __init taskdtl_init(void)
{
	struct pid         *pid_data;
	struct task_struct *task;

	if (pid <= 0 || pid > PID_MAX_LIMIT) {
		pr_err("PID %d out of range\n", pid);
		return -EINVAL;
	}

	pid_data = find_get_pid(pid);
	if (!pid_data) {
		pr_err("PID %d not found\n", pid);
		return -ESRCH;
	}

	task = get_pid_task(pid_data, PIDTYPE_PID);
	if (!task) {
		pr_err("no task found for PID %d\n", pid);
		put_pid(pid_data);
		return -ESRCH;
	}

	process_task(task);

	return 0;
}

static void __exit taskdtl_exit(void)
{
	pr_info("taskdtl misc driver exit\n");
}

module_init(taskdtl_init);
module_exit(taskdtl_exit);
