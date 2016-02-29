			Projet NMV - Simulation des commandes du noyau Linux 
				
Etudiant: Minh-Hieu PHAM et Jonathan Espié--Caullet
Master SAR - 2015/2016

Ce Readme a pour but de documenter notre travail dans le cadre de la matière Noyau-Multicoeurs-Virtualisation (NMV). La documentation commence par la présentation du projet puis présente le déroulement de chaque commande implémentée.  
	
Remerciement: Au terme de notre travail, nous tenons à témoigner notre profonde reconnaissance à toutes les personnes dont nous avons sollicité la compétence et l’expérience.  Nous remercions vivement Monsieur Julien Sopéna, Professeur à l’Université Pierre et Marie Curie, Monsieur Maxime Lorrillere ainsi que nos camarades en SAR. Nous souhaitons leur exprimer notre gratitude pour les connaissances qu’ils ont partagées, et les encouragements et la confiance qu’ils nous ont toujours témoignés. 
	
1. Présentation générale: 

	L'objectif du projet est de réaliser un outil en ligne de commande capable d'interagir avec le noyau pour obtenir certaines informations ou exécuter certaines actions en manipulant des conceptions de work_queue et wait_queue. L'implémentation reçoit des commandes par l'intermédiaire d'un module. Le programme d'expérimentation utilisateur utilse des ioctl pour la communication entre l'outil et le module. L'implémentation d'une telle fonctionnalité fournit des mécanismes permettant d'exécuter du code dans le noyau de façon asynchrone et synchrone. 
	
	La suite du document va vous présenter de manière détaillée les commandes  que nous avons implémenté notamment kill - envoyer un signal à un processus, waitall - attendre en bloquant la terminaison de tous les processus donnés, wait attendre en bloquant la terminaison d'un des processus donnés, print meminfo - afficher les informations sur l'état de la mémoire, lsmod - renvoyer la liste des modules chargés dans le noyau. 
	
	Instructions d'implémentation:
	- Application du patch kernel.patch sur un noyau linux version 4.2.3 et compilation du noyau
	- Insertion d'un module dans le noyau: "insmod projet_nmv.ko"
	- Récupération du numéro majeur généré par le module: major_number affiché dans KGBD
	- Création d'un périphérique "mknod /dev/projet_nmv c major_number 0" (par défaut, on a assigné le nom du périphérique à "projet_nmv")
	- Lancement du programme d'utilisateur et examination des commandes
	
2. Commandes implémentées

	a. kill <signal> <pid>
		
	La commande prend deux paramètre: un numéro de signal et un pid, et envoie, par l'intermédiaire du module, le signal correspondant au processus de pid indiqué (s'il existe). 
		
	En recevant la commande envoyée par l'utilisateur via le programme de test (user_request.c) implémentant ioctl, le module délègue le traitement à la fonction driver_cmd. Elle insère tout de suite la tâche nmv_kill, grâce à la fonction schedule_work, dans la work_queue qui sera traitée par un thread du noyau qui a pour mission d'exécuter des tâches de la work_queue. La fonction nmv_kill est appelée: elle verifie l'existance du processus qui a le numéro donné -  find_get_pid; et essaye d'envoyer le signal à ce processus - kill_pid. 		
		
	kill fonctionne en deux mode: synchrone et asynchrone. 
	- kill en mode synchrone: Le module s'endort sur la wait_queue dans l'attente que le signal soit envoyé au processus - wait_event(wait_queue, kill_cond). Une fois que la fonction nmv_kill finit son travail, elle change la valeur de la variable kill_cond et réveille tous les processus dans la wait_queue (ici c'est le programme principal du module) via l'appel de la fonction wake_up. Une autre solution est d'utiliser la fonction flush_work pour forcer l'exécution d'une tâche de la work_queue et d'attendre qu'elle se termine.  
	- kill en mode asynchrone: Le module retourne en état "normal" (et attend l'arrivée d'autres commandes) après avoir délégué le traitement de la fonction nmv_kill par le noyau (insérée dans le work_queue). 
	
	Expérimentation: 
	- Compiler et exécuter le programme proc_test.c avec l'option &. 
	- Essayer de "kill" les processus proc_test et observer le résultat.
	- Vérifier la terminaison des processus par la command ps -aux 
	
	b. print meminfo

	La commande permet d'obtenir des informations sur l'état de la mémoire. Au sein du noyau Linux, on utilise la fonction si_meminfo pour obtenir ces informations. 
		
	En recevant la commande envoyée par l'utilisateur, le programme principal du module (driver_cmd) délègue le traitement à la fonction nmv_meminfo. Après avoir récupéré les informations sur l'état de la mémoire via la fonction si_meminfo, nmv_meminfo les garde dans une structure de type sysinfo. Afin de simplifier le programme, on a décidé de générer un message de type de chaîne de caractères qui contient les informations de la mémoire (le champs char *memInfoStr de la structure meminfo_command) et de l'envoyer à l'utilisateur via la fonction copy_to_user (le main du traitement du module). 
		
	En acceptant le message renvoyé par le module, le programme de test (user_request) affiche les informations sur l'état de la mémoire. 

	meminfo fonctionne en deux modes "synchrone" et "asynchrone"
	- En mode synchrone, le programme utilisateur lance la requête NMV_LSMOD et attend la réponse du côté de "noyau" (module implémenté). En recevant la requête NMV_LSMOD, le module récupère les informations de la mémoire et renvoie au programme utilisateur. Le programme utilisateur reçoit la réponse du module et termine sa tâche. 
	- En mode asynchrone, le programme utilisateur lance la requête NMV_LSMOD et attend la réponse du côté de "noyau". Au lieu de traiter la requête puis envoyer la réponse, le noyau renvoie tout de suite un message "Processing meminfo.... Please call NMV_ASYN to receive the result" au programme utilisateur puis continue de traiter la requête. Afin de simuler de manière approximative, dans notre projet, nous avons ajouté un délai de 3 secondes pour chaque traitement: 
		+ set_current_state(TASK_INTERRUPTIBLE);
		+ schedule_timeout(3*HZ);
	+ à la fin du traitement, le noyau remplit une chaîne caractère <<result>> qui contient la réponse de la requête. 
		+ strcat(result, memInfoStr);
	+ pour que le programme utilisateur puisse savoir la réponse de sa requête, il devrait appeler une deuxième requête NVM_ASYN pour lire la réponse qui était "stocké" dans la chaîne <<result>>. 
		+ printf("Calling NMV_ASYN......\n");
		+ ioctl(f, NMV_ASYN, &response);
		+ printf("->Meminfo: \n%s", response);
	+ Dans le cadre du projet, on considère que pour le moment, il n'y a qu'un seule programme utilisateur et que l'utilisateur ne lance qu'une seule requête. 
	
	c. lsmod 

	La commande renvoie une liste des modules chargés dans le noyau (noms, compteur de références, taille, ...). 
		
	La commande lsmod est implémentée de manière similaire à la commande meminfo documentée ci-dessus. Nous avons dû modifier et recompiler le noyau afin d'exporter la liste des modules qui est défini comme une variable globale dans le noyau. Cette modification est détaillée dans le fichier kernel.patch

	Comme la commande meminfo, La commande lsmod fonctionne en deux mode asynchrone (avec &) et synchrone.   
		
		======
		diff -Naur linux-4.2.3-orig/include/linux/module.h linux-4.2.3/include/linux/module.h
		--- linux-4.2.3-orig/include/linux/module.h	2015-10-03 13:52:18.000000000 +0200
		+++ linux-4.2.3/include/linux/module.h	2016-01-11 17:58:45.299199829 +0100
		@@ -487,6 +487,7 @@
		#endif
 
		extern struct mutex module_mutex;
		+extern struct list_head modules;
 
		/* FIXME: It'd be nice to isolate modules during init, too, so they
		aren't used before they (may) fail.  But presently too much code
		diff -Naur linux-4.2.3-orig/kernel/module.c linux-4.2.3/kernel/module.c
		--- linux-4.2.3-orig/kernel/module.c	2015-10-03 13:52:18.000000000 +0200
		+++ linux-4.2.3/kernel/module.c	2016-01-11 17:59:21.694659166 +0100
		@@ -100,7 +100,8 @@
		* (delete and add uses RCU list operations). */
		DEFINE_MUTEX(module_mutex);
		EXPORT_SYMBOL_GPL(module_mutex);
		-static LIST_HEAD(modules);
		+LIST_HEAD(modules);
		+EXPORT_SYMBOL_GPL(modules);
 
		#ifdef CONFIG_MODULES_TREE_LOOKUP
		======
		
	d. waitall <pid> [<pid> ...] et wait <pid> [<pid> ...]

	En recevant la liste des pids, le programme utilisateur détermine pour quel mode wait ou waitall en construisant la structure wait_event qui correspond (en indiquant wait_all à true ou false). Le module du noyau crée ensuite une liste des processus nmv_wait_list qu'on doit attendre (fonction add_process) en vérifiant qu'ils existent et qu'ils soient en vie à ce moment. Dans la fonction principale du traitement : nmv_blocking_wait, on vérifie régulièrement la vie des processus via la fonction is_process_alive en parcourant la liste des processus considérés. Si un processus est mort, il est retiré de la liste des processus vivants. Dépendant l'option d'attente (wait ou waitall), le module termine le traitement. 

	Afin de mieux observer les processus vivants, on récupère le code de la fonction monitor_pid des TPs pour afficher chaque 1 seconde un message pour vérifier si un processus est toujours vivant. 
	
	Les commandes wait et waitall fonctionnent en deux mode synchrone et asynchrone de même mécanisme avec les commandes kill, meminfo et lsmod. 
		
3. Conclusion
	Dans le cadre du projet NMV, les résultats peuvent être résumés en trois points décrits ci-dessous : 

	- Premièrement, à partir du sujet, on a formulé la problématique du traitement des commandes kill, wait, meminfo et lsmod du noyau. 
	- Puis, dans un deuxièmement temps, nous avons modélisé les structures de donnée de commandes embarquant la structure struct struct_work tout en montrant son avantage par rapport à nôtre problématique.
	- Enfin, nous avons implémenté le projet en profitant des connaissances acquises et des travaux effectués pendant le TP/TME notamment  Monitoring de l’activité d’un processus (TP05), Gestion de la mémoire dans le noyau (TP06), Traitement des ioctl, ect. Les démonstrations et leurs explications expérimentales prouvent que l’implémentation est cohérente avec la conception. 

	La réalisation de ce projet nous a permis d’acquérir une expérience dans la gestion des tâches afin de respecter le temps imparti dédié à la réalisation de ce travail en groupe. Notre compétences de programmation du noyau, surtout concernant la gestion des ressources du noyau se sont nettement améliorées grâce à ce projet. 
	
	En réalisant le projet, on rencontre la problématique de multi-utilisation. C'est-à-dire, pour un moment, s'il y a deux requêtes asynchrone par deux utilisateurs différents envoyées au noyaux et qu'on n'a qu'une seule variable <<result>> pour stocker la réponse, on n'est pas capable de "identifier" quelle requête termine avant. Une des solutions proposées est d'implémenter le mécanisme multithreading pour travailler avec chaque requête reçu.  
	
	Il reste cependant des points à améliorer, notamment concernant l’optimisation du code et de la gestion efficace d'allocation de la mémoire. Une perspective du projet serait de mieux connaitre le savoir-faire sur l'implémentation et l'utilisation d'un propre work_queue indépendamment la work_queue du noyau. Le projet soulève une autre perspective d'implémenter le mécanisme multithreading pour répondre la problématique muli-utilisation posée au-dessus. 
