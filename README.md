# Jeu d’Awalé

#### Programmation Réseaux


## Compilation des fichiers sources

Pour compiler le code source :

1. Ouvrir un terminal dans le répertoire racine du projet.

2. Générer les fichiers build : `cmake CMakeLists.txt`

3. Build (compiler) le project : `make`

Cela produit deux fichiers exécutables : ___awale_server___ et ___awale_client___.

Pour démarrer le jeu :

1.	Démarrer le serveur (exécuter ___awale_server___)
2.	Démarrer le jeu (exécuter ___awale_client___)


## Les fonctionnalités implémentées

Vous vous connectez d'abord en donnant votre nom d'utilisateur.

Celui-ci vous identifie de manière unique et est stocké sur le serveur de jeu afin que vous puissiez partir et revenir comme vous le souhaitez pour reprendre les jeux.

Une fois connecté, vous pouvez effectuer plusieurs actions :

- `list`       - liste de tous les joueurs actuellement en ligne
- `challenge`  - défier un joueur (pour commencer une nouvelle partie, affiche un message d'erreur mais crée bien une partie)
- `respond`	   - répondre aux défis des autres (non implémenté)
- `play`       - commence ou reprendre un jeu
- `quit`       - se déconnecter et quitter (équivalent à terminer le programme)

Vous pouvez effectuer ces actions en saisissant la commande correspondante.


## Le jeu

- Jouez en selectionnant la case dont vous souhaitez deplacer les pierres.

- Le calcul des points et de la victoire est automatique, vous pouvez cependant abandonner en cours de partie lors de votre tour.
