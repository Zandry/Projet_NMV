					Projet NMV - Simulation des commandes du noyau Linux 
				
				Etudiant: Minh-Hieu PHAM et Jonathan Espié--Caullet
				Master SAR - 2015/2016

	Ce Readme a pour but de documenter notre travail dans le cadre de la matière Noyau-Multicoeurs-Virtualisation (NMV). La documentation commence par la présentation du projet puis on va mettre en détail le déroulement de chaque commande implémentée.  
	
	Remerciement: Au terme de notre travail, nous tenons à témoigner notre profonde reconnaissance à toutes les personnes dont nous avons sollicité la compétence et l’expérience.  Nous remercions vivement Monsieur Julien Sopéna, Professeur à l’Université Pierre et Marie Curie, Monsieur Maxime Lorrillere ainsi que notre camarades en SAR. Nous souhaitons leur exprimer notre gratitude pour les connaissances qu’ils ont partagées, et les encouragements et la confiance qu’ils nous ont toujours témoignés. 
	
1. Présentation générale: 

	L'objectif du projet est de réaliser un outil en ligne de commande capable d'interagir avec le noyau pour obtenir certaines informations ou exécuter certaines actionn en manipulant des conceptions de workqueue et waitqueue. L'implémentation reçoit des commandes par l'intermédiaire d'un module. Le programme d'expérimentation utilse des ioctl pour la communication entre l'outil et le module. L'implémentation d'une telle fonctionnalité fournit des mécanismes permettant d'exécuter du code dans le noyau de façon asynchrone et synchrone. 
	
	La suite du document va vous présenter de la manière détaillée des commandes  du noyau notamment kill - envoyer un signal à un proccessus, waitall - attendre en bloquant la terminaison de tous les processus donnés, wait attendre en bloquant la terminaison d'un des processus donnés, print meminfo - afficher les informations sur l'état de la mémoire, lsmod - renvoyer la liste des modules chargés dans le noyau. 
	
2. Commandes implémentées

	a. kill <signal> <pid>
		
		La commande prend deux paramètre: un numéro de signal et un pid, et envoie, par l'intermédiaire du module, le signal correspondant au processus de pid indiqué (s'il existe). 
		
		En recevant la commande envoyée par l'utilisateur via le programme de test (user_request.c) implémentant ioctl, le modulé délègue le traitement à la fonction driver_cmd. Elle insère tout de suite le tâche, grâce à la fonction schedule_work, dans le workqueue - un thread du noyau qui a pour mission d'exécuter des tâches de manière asynchrone. La fonction nmv_kill est appélé: vérifier l'existance du proccessus qui a le numéro donné -  find_get_pid; essayer d'envoyer le signal à ce proccessus - kill_pid, terminer le thread dans workqueue et retourner au programme principal du module. 		
		
		kill fonctionne en deux mode: synchrone et asynchrone. 
		- kill en mode synchrone: Le module s'endort sur le wait_queue dans l'attente que le signal est bien envoyé au proccessus - wait_event(wait_queue, kill_cond). Une fois que la fonction nmv_kill finit son travail, il change la valeur de la variable kill_cond et réveille "tout" les processus (ici c'est le programme principal du module) via l'appel de la fonction wake_up. Une autre solution est d'utiliser la fonction flush_work pour forcer l'exécution d'une tâche et d'attendre qu'elle se termine.  
		- kill en mode asynchrone: Le module retourne en état "normal" (attendre d'autres commandes arrivées) après déléguer le traitement à la fonction nmv_kill (insérer dans le work_queue). 
		
	b. waitall <pid> [<pid> ...]
	c. wait <pid> [<pid> ...]
	d. print meminfo
		La commande permet d'obtenir des informations sur l'état de la mémoire. Au sein du noyau Linux, on utilise la fonction si_meminfo pour obtenir ces informations. 
		
		En recevant la commande envoyée par l'utilisateur, le programme principal du module (driver_cmd) délègue le traitement à la fonction nmv_meminfo. Après récupérer les information sur l'état de la mémoire via la fonction si_meminfo, nmv_meminfo les garde dans la structure de type sysinfo. Afin de symplifier le programme, on a décidé de générer un message de type de chaîne de caractères qui contient les informations de la mémoire (le champs char *memInfoStr de la structure meminfo_command) et l'envoyer à l'utilisateur via la fonction copy_to_user (le main du traitement du module). 
		
		En acceptant le message renvoyé par le module, le programme de test (user_request) affiche les informations sur l'état de la mémoire. 
		
	e. lsmod 
		La commande renvoie une liste des modules chargés dans le noyau (noms, compteur de références, taille, ...). 
		
		La commande lsmod est implémentée de la manière similaire avec la commande meminfo documenté ci-dessus. Particulièrement, on devrait modifier et recompiler le noyau afin d'exporter la liste des modules qui est défini comme une variable globale dans le noyau. Cette modification est détaillée dans le fichier kernel.patch
		
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
		
3. Conclusion
	Dans le cadre du projet NMV, les résultats peuvent être résumés en trois points décrits ci-dessous : 

	- Premièrement, à partir du sujet, on a formulé la problématique du traitement des commandes kill, wait, meminfo et lsmod du noyau. 
	- Puis, dans un deuxièmement temps, nous avons modélisé les structures de donnée de commandes embarquant la structure struct struct_work tout en montrant son avantage par rapport à nôtre problématique.
	- Enfin, nous avons implémenté le projet en profitant des connaissances acquises et des travaux effectués pendant le TP/TME notamment  Monitoring de l’activité d’un processus (TP05), Gestion de la mémoire dans le noyau (TP06), Traitement des ioctl, ect. Les démonstrations et leurs explications expérimentales prouvent que l’implémentation est cohérente avec la conception. 

	La réalisation de ce projet nous a permis d’acquérir une expérience dans la gestion des tâches afin de respecter le temps imparti dédié à la réalisation de ce travail en groupe. Notre compétences de programmation du noyau, surtout concernant la gestion des ressources du noyau se sont nettement améliorées grâce à ce projet. 

	Il reste cependant des points à améliorer, notamment concernant l’optimisation du code et de la gestion efficace d'allocation de la mémoire. Une autre perspective du projet serait de mieux connaitre le savoir-faire sur l'implémentation et l'utilisation d'un propre workqueue indépendamment le workqueue du noyau.
