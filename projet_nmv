#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/workqueue.h>
#include <linux/kthread.h>
#include <linux/kobject.h>
#include <linux/uaccess.h>
#include <linux/fs.h>
#include <linux/signal.h>
#include <linux/decompress/mm.h>
#include <linux/mm.h>
#include <linux/list.h>

#include <linux/ioctl.h>

#define NMV_KILL _IOR('N',0,void*)
#define NMV_WAIT _IOR('N',1,void*)
#define NMV_LSMOD _IOW('N',2,void*)
#define NMV_MEMINFO _IOW('N',3,void*)

#define BUFFER_SIZE 1024

MODULE_DESCRIPTION("A projet_nmv kernel module");
MODULE_AUTHOR("Jonathan - Minh-Hieu");
MODULE_LICENSE("GPL");

DECLARE_WAIT_QUEUE_HEAD(wait_queue);

struct file_operations ops;
int major_num = 0;
const char *name = "projet_nmv";
bool kill_cond = false; 
bool wait_cond = false; 

struct kill_event {
	int signal; 
	pid_t pid_nr; 
	bool syn; 
	/*
		true: synchrone
		false: asynchrone
	*/
};

struct lsmod_command {
	char *lsmod_print; /* To print in user terminal */
	struct work_struct task;
};

struct kill_command {
	struct kill_event event;
	struct work_struct task;
};

struct meminfo_command {
	 /* To print in user terminal */
	char *memInfoStr;
	struct work_struct task; 
};

struct wait_event {
	int nb_pid;
	bool wait_all;
	pid_t * pid_nr;
};

struct wait_command {
	struct wait_event event;
	struct list_head head;
	struct work_struct task;
};

struct nmv_wait_list
{
	pid_t pid_nr;
	struct pid * pid_info;
	struct task_struct * task_info;
	struct list_head list;
};



struct kill_command kill_command_execute;
struct wait_command wait_command_execute;
struct lsmod_command lsmod_command_execute;
struct meminfo_command meminfo_command_execute; 

struct pid * monitor_pid(pid_t nr)
{
	struct pid * s = find_get_pid(nr);
	
	if (s == NULL)
		pr_warn("Processus %d introuvable !\n", nr);
	else
		pr_warn("Processus %d trouve !\n", nr);
	return s;
}

/* Kill command */
static void nmv_kill(struct work_struct *wk){
	struct kill_command *kc; 
	struct pid *p;
	
	pr_info("Process a kill command");
	kc = container_of(wk, struct kill_command, task); 
	/* Find the pid */
	p = monitor_pid(kc->event.pid_nr);
	if(p != NULL){
		if(kill_pid(p, kc->event.signal, 0) != 0) {
		pr_err("Fail to send sig to pid %d!\n", (int)kc->event.pid_nr);
		put_pid(p);
		}
	}
	else{
		pr_err("Pid %d not found!\n", kc->event.pid_nr);
	}
	kill_cond = true; 
	wake_up(&wait_queue); 
}

/* lsmod command */
static void nmv_lsmod(struct work_struct *wk){
	struct lsmod_command *lc;
	struct module *mod;
	char *module_info;
	int mem_free, char_append;

	pr_info("Process a lsmod command");
	mem_free = BUFFER_SIZE;
	char_append = 0;
	lc = container_of(wk, struct lsmod_command, task);
	lc->lsmod_print = kmalloc(BUFFER_SIZE, GFP_KERNEL);
	list_for_each_entry(mod, &modules, list) {
		module_info = kmalloc(BUFFER_SIZE, GFP_KERNEL);
		mem_free -= scnprintf(module_info, BUFFER_SIZE,
				      "\tModule : %s Size : %8u Version :%s",
				      mod->name, mod->core_size, mod->version);
		strncat(module_info, "\n", 1);
		strcat(lc->lsmod_print, module_info);
		kfree(module_info);
	}
}

/* meminfo command */
static void nmv_meminfo(struct work_struct *wk){
	struct meminfo_command *mw;
	char *memInfoStr;
	struct sysinfo memInfo;
	
	pr_info("Process a meminfo command");
	mw = container_of(wk, struct meminfo_command, task);
	kfree(mw->memInfoStr);
	si_meminfo(&memInfo);
	memInfoStr = kmalloc(BUFFER_SIZE, GFP_KERNEL);
	scnprintf(memInfoStr, BUFFER_SIZE,
				      "\tTotal usable main memory size : %ld\n\tAvailable memory : %ld\n\tUsed memory : %ld",
				      memInfo.totalram, memInfo.freeram, memInfo.totalram - memInfo.freeram);
	strncat(memInfoStr, "\n", 1);
	mw->memInfoStr = kmalloc(BUFFER_SIZE, GFP_KERNEL);
	strcpy(mw->memInfoStr, memInfoStr);
	kfree(memInfoStr);
}


/* Wait command */
bool is_process_alive (struct nmv_wait_list *process)
{
	if (process == NULL || process->pid_info == NULL || process->task_info == NULL)
		return false;//Processus non trouvé

	if (pid_alive(process->task_info)){
		pr_info("Processus %d en vie !\n", process->pid_nr);
		return true;
	}

	pr_warn("Processus %d mort !\n", process->pid_nr);
	return false;
}

struct nmv_wait_list * add_process(struct list_head *head, pid_t new_pid)
{//Cree et ajoute le processus de pid new_pid dans la liste processes
	struct pid * new_pid_info;
	struct task_struct * new_task_info;
	struct nmv_wait_list * new_process;

	new_pid_info = monitor_pid(new_pid);
	if (new_pid_info == NULL)
		return NULL;

	new_task_info = get_pid_task(new_pid_info, PIDTYPE_PID);
	
	if (new_task_info == NULL)
		return NULL;

	new_process = (struct nmv_wait_list *)kmalloc(sizeof(struct nmv_wait_list), GFP_KERNEL);

	new_process->pid_nr = new_pid;
	new_process->pid_info = new_pid_info;
	new_process->task_info = new_task_info;
	INIT_LIST_HEAD(&(new_process->list));

	if (head != NULL)
		list_add_tail(&(new_process->list), head);

	return new_process;
}


void create_wait_list(struct list_head *head, struct wait_event * event)
{
	int i;
	INIT_LIST_HEAD(head);
	
	if (event->nb_pid > 0)
	{
		for (i = 0 ; i < event->nb_pid ; i++)
		{
			add_process(head, event->pid_nr[i]);
		}
	}
}

void nmv_blocking_wait(struct list_head *head, bool all)
{//Prend en parametre une liste de processus vivants et si on doit attendre un ou tout les processus
	int nb_alive_processes;
	int nb_target_alive_processes = 0;
	struct nmv_wait_list * current_ptr;
	struct nmv_wait_list * safe_ptr;

	pr_info("Begin NMV_Wait !\n");

	list_for_each_entry(current_ptr, head, list)
	{
		nb_target_alive_processes++;
	}
	
	pr_info("Monitoring %d process(es) !\n", nb_target_alive_processes);

	if (all)
		nb_target_alive_processes = 0;//On doit attendre jusqu'a ce que tout les processus soient termines
	else if (nb_target_alive_processes > 0)
		nb_target_alive_processes--;//On doit attendre jusqu'a ce que un des processus se termine

	do
	{
		nb_alive_processes = 0;
		list_for_each_entry_safe(current_ptr, safe_ptr, head, list)
		{
			if (is_process_alive(current_ptr))
				nb_alive_processes++;
			else
			{//On enleve le processus de la liste des processus vivants
				list_del(&(current_ptr->list));
				put_pid(current_ptr->pid_info);
				free(current_ptr);
			}
		}

		set_current_state(TASK_INTERRUPTIBLE);
		schedule_timeout(1*HZ);
	}
	while (nb_alive_processes > nb_target_alive_processes);

	if (all)
		pr_info("End NMV_Wait : All processes are dead !\n");
	else
		pr_info("End NMV_Wait : A process is dead !\n");

	if (nb_alive_processes > 0)//Il faut vider la liste si il reste des processus dedans
	{
		list_for_each_entry_safe(current_ptr, safe_ptr, head, list)
		{
			list_del(&(current_ptr->list));
			put_pid(current_ptr->pid_info);
			free(current_ptr);
		}
	}

	return; 
}

static void nmv_wait(struct work_struct *wk)
{
	struct wait_command *wc;

	wc = container_of(wk, struct wait_command, task);
	
	create_wait_list(&(wc->head), &(wc->event));
	nmv_blocking_wait(&(wc->head), wc->event.wait_all);//on attend les processus donnes

	wait_cond = true; 
	wake_up(&wait_queue); 
}


long driver_cmd (struct file * f, unsigned int requete, unsigned long param)
{	
	switch(requete)
	{
		case NMV_KILL:
			copy_from_user(&(kill_command_execute.event), (void *)param, sizeof(struct kill_event));
			pr_info("Received IOCTL NMV_KILL : Pid : %d\n", kill_command_execute.event.pid_nr);
			schedule_work(&(kill_command_execute.task));
			if(kill_command_execute.event.syn == true)    /* Synchrone */
				wait_event(wait_queue, kill_cond);//flush_work(&(kill_command_execute.task));
			kill_cond = false;
			
			break;

		case NMV_LSMOD:
			pr_info("Received IOCTL NMV_LSMOD\n");
			schedule_work(&(lsmod_command_execute.task));
			flush_work(&(lsmod_command_execute.task));
			pr_info("%s", lsmod_command_execute.lsmod_print);
			copy_to_user((char *)param, lsmod_command_execute.lsmod_print , BUFFER_SIZE);
			break;

		case NMV_WAIT:
			copy_from_user(&(wait_command_execute.event), (struct wait_event *)param, sizeof(struct wait_event));
			pr_info("Received IOCTL NMV_WAIT : %d processes, %d wait_all !\n", wait_command_execute.event.nb_pid, wait_command_execute.event.wait_all);
			//create_wait_list(&(wait_command_execute.head), &(wait_command_execute.event));
			schedule_work(&(wait_command_execute.task));
			wait_event(wait_queue, wait_cond);//flush_work(&(wait_command_execute.task));
			wait_cond = false;
			break;
		
		case NMV_MEMINFO:
			pr_info("Received IOCTL NMV_MEMINFO\n");
			schedule_work(&(meminfo_command_execute.task));
			flush_work(&(meminfo_command_execute.task));
			pr_info("%s", meminfo_command_execute.memInfoStr);
			copy_to_user((char *)param, meminfo_command_execute.memInfoStr, BUFFER_SIZE);
			break;
		
		default:
			pr_err("Bad command !\n");
			return -ENOTTY;
			break;
	}
	return 0;
}

static int projet_nmv_init(void)
{

	pr_warn("projet_nmv module loaded\n");

	ops.unlocked_ioctl = driver_cmd;
	
	major_num = register_chrdev(0, name, &ops);

	if(!major_num) {
		pr_err("Failed to set major number!\n");
		return -1;
	}
	
	pr_info("Driver major num : %d\n", major_num);

	//meminfo_command_execute = (struct meminfo_command*)kmalloc(sizeof(struct meminfo_command), GFP_KERNEL);
	/* Init the task for the workqueue */
	INIT_WORK(&(kill_command_execute.task), nmv_kill);
	INIT_WORK(&(lsmod_command_execute.task), nmv_lsmod);
	INIT_WORK(&(wait_command_execute.task), nmv_wait);
	INIT_WORK(&(meminfo_command_execute.task), nmv_meminfo);
		
	return 0;
}

static void projet_nmv_exit(void)
{
	/*kfree(&kill_command_execute);
	kfree(&lsmod_command_execute);
	kfree(&wait_command_execute); 
	kfree(meminfo_command_execute);*/
	unregister_chrdev(major_num, name);

	pr_warn("projet_nmv module unloaded\n");
}

module_init(projet_nmv_init);
module_exit(projet_nmv_exit);
