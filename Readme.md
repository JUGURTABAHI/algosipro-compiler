# Projet de Compilation : Compilateur AlgoSipro

* **Cadre du projet :** UE de Compilation -
                        Université de Rouen Normandie.

* **Année universitaire :** L3 INFORMATIQUE.
* **Réalisé par :** BAHI Jugurta.
* **Sous la direction de :** M. LEFEBVRE Arnaud.


# Ce projet implémente un compilateur complet développé en **C**, utilisant
# **Flex (Analyseur Lexical)** et **Bison (Analyseur Syntaxique)**. Il prend
# en entrée des algorithmes écrits dans une syntaxe inspirée de LaTeX (suivis
# d'un appel final), et génère du code assembleur optimisé destiné à être
# exécuté par la machine virtuelle **sipro**.

### Capacités du Compilateur
* **Multi-algorithmes** :
    - Supporte la déclaration et l'enchaînement de plusieurs blocs 
    \begin{algo} ... \end{algo}.
* **Récursivité** : 
    - Gère les appels récursifs (un algorithme qui s'appelle lui-même) 
    ainsi que les appels croisés.
* **Structures de contrôle** : 
    - Prise en charge complète des conditionnelles (\IF) et des boucles
    (\DOWHILE, \DOFORI).
* **Point d'entrée** : Exécution déclenchée par l'appel \CALL final en fin de 
    fichier.
                       
### Blocs de Contrôle
* **Assignation :** 
    - `\SET{nom}{valeur}`
    Permet d'initialiser une variable locale, 
    ou de la modifier (a condition!). 
* **Conditions :** 
    - `\IF{condition} ... \ELSE ... \FI`
    Structure de choix logique. Le bloc `\ELSE` est optionnel.
* **Boucles :** 
    - `\DOWHILE{cond} ... \OD` : 
        - Boucle tant que (test au début).
    - `\DOFORI{i}{min}{max} ... \OD` : 
        - Boucle itérative de min à max inlcus.
        - Les bornes min et max acceptent des expressions complexes, 
        comme un `\CALL` renvoyant un entier.

### Gestion des Algorithmes
* **Définition :**
    - `\begin{algo}{nom}{param1, param2, ....., paramN} ... \end{algo}`
    Déclare un bloc de code réutilisable. 
    Le corps doit contenir au moins une instruction.
* **Retour de valeur :**
    - `\RETURN{expression}`
    Indique la valeur produite par l'algorithme 
    et interrompt immédiatement son exécution.
    La valeur renvoyée par l'appel final est 
    affichée sur la sortie standard avec `"\n"`.

### Appel de Fonctions (\CALL)
* **Usage comme Expression :**
    - `\SET{x}{\CALL{fun}{arg1, arg2, ...., argN}}`
    L'appel peut être utilisé à l'intérieur d'une assignation. 
    Il exécute l'algorithme cible et récupère sa valeur de 
    retour pour l'assigner à une variable.
* **Usage comme Instruction (Point d'entrée) :**
    - `\CALL{fun}{arg1, arg2, ...., argN}`
    Placé seul en fin de fichier, il sert de point d'entrée. 
    C'est lui qui déclenche l'exécution globale du programme. 
    S'il est absent, la compilation n'aura pas lieu.

### Entrées / Sorties
* **Affichage :**
    - `\PRINTD{expression}` : Affiche un entier ou le résultat d'un calcul.
    sans `"\n"`.
    - `\PRINTS{"texte"}` : Affiche une chaîne de caractères brute (string).

### Sûreté et Typage
* **Contraintes de Type :**
    - Seuls les deux types sont supportés : `INT` et `BOOL`.
    - Les types `INT` et `BOOL` sont strictement incompatibles. 
    - Le compilateur refuse d'assigner un résultat de comparaison 
      (booléen) à une variable entière, et inversement.
    - Les paramètres des fonctions sont exclusivement de type INT. Les fonctions
      peuvent retourner un INT ou un BOOL. Aucune conversion implicite entre ces
      deux types n’est autorisée.
* **Gestion des Identifiants :**
    - Une erreur est levée si l'on tente d'appeler une fonction 
      non déclarée ou d'utiliser une variable inexistante.
    - Les variables sont strictement locales à chaque algorithme.

### Opérateurs et Expressions
Le compilateur supporte une large gamme d'opérateurs pour les calculs 
et les tests logiques :

* **Arithmétique :** 
    - `+`, `-`, `*`, `/`.
* **Comparaison :** 
    - `==` (ou `=`), `!=`, `<`, `>`, `<=`, `>=`.
* **Logique :** 
    - `&&` (ET), `||` (OU).
* **Commentaires :** 
    - **Lignes simples :** Le compilateur ignore les commentaires de forme (`%` 
    et `#`) ainsi que les syntaxes standard (`//` et `/* ... */` lorsqu'ils sont
    sur une seule ligne).
    - **Blocs multilignes :** Support complet des blocs de commentaires 
    encadrés par `\COM` et `\ENDCOM`. 
    - **Sûreté technique :** Ces blocs utilisent un état exclusif 
    dans Flex %x COMMENT

* **Parenthésage :** 
    - Le compilateur supporte les parenthèses `(` et `)` pour 
    forcer la priorité des opérations dans les calculs complexes.

## Optimisations de l'Arbre Syntaxique (AST)

Le compilateur intègre un moteur d'optimisation activable via le flag 
`-DOPTIMIZE` dans le `Makefile`. Ces transformations interviennent après le 
parsing mais avant la génération de l'assembleur.

###             1. Inlining Universel (Substitution de Fonctions)
Toute fonction se résumant à un unique bloc `\RETURN` est `inlinée`. 
* **Le concept :** 
    - L'appel `\CALL` est remplacé directement par le corps de la
    fonction dans `l'AST`.
* **Impact :** 
    - Réduire la complexité l'appel (pas de `push bp`, `call`....). 
* **Support :** 
    - Fonctionne pour `0`, `1` ou `N` paramètres grâce à la
    substitution récursive des identifiants.

###             2. Constant Folding (Propagation de Constantes)
Évaluation des expressions constantes lors de la compilation.
* **Exemple :** 
    - `\SET{x}{2 + 3 * 4}` est transformé directement 
    en `NODE_VALUE : 14`.
* **Opérateurs gérés :** 
    - Arithmétique (`+`, `-`, `*`, `/`) et Comparaisons (`<`, `>`, `==`, etc.).

###             3. Propagation de Constantes (Constant Propagation)
Lorsqu'une variable est définie par une constante (ex: \SET{a}{5}), 
le compilateur remplace les occurrences suivantes de cette variable
par sa valeur directe dans l'arbre.
Cette étape est cruciale car elle "débloque" le Constant Folding sur
des expressions qui semblaient dynamiques au départ.

###             4. Simplifications Algébriques
Réduction des expressions mathématiques pour éliminer les calculs inutiles :
* **Éléments neutres :** 
    - `x + 0` ou `0 + x`  -->  `x`
    - `x * 1` ou `1 * x`  -->  `x`
    - `x - 0`             -->  `x`
    - `x / 1`             -->  `x`
* **Éléments absorbants :** 
    - `x * 0` ou `0 * x`  -->  `0`
    - `0 / x`             -->  `0`
* **Identités :** 
    - `x - x`             -->  `0`
    - `x / x`             -->  `1`
* **Associativité (Advanced Folding) :** 
    - `(x + C1) + C2`  -->  `x + (C1 + C2)`
    - `(C1 + x) + C2`  -->  `x + (C1 + C2)`
    - `C1 + (x + C2)`  -->  `x + (C1 + C2)`
    - *Exemple :* `(x + 5) + 10` est optimisé directement en `x + 15`.

* **Division par zéro :**
Si le dénominateur est une constante `0`, le compilateur lève une erreur et 
interrompt immédiatement la compilation.

###             5. Optimisations de Boucles & Contrôle
* **Loop Pruning :** 
    - Suppression intégrale des boucles `FOR` dont les bornes constantes 
    sont invalides (ex: `10` à `5`).

* **Branch Pruning :** 
    - Si une condition de `\IF` est constante, seule la branche active est
    conservée. La structure de contrôle et la branche inutile sont détruites.

###             6. Élimination du Code Mort (Dead Code Elimination)
* **Fonctions Orphelines :** 
    - Suppression des algorithmes jamais appelés ou devenus inutiles après
    la phase d'inlining.

* **Unreachable Code :** Analyse récursive du flux de contrôle pour supprimer 
    - toute instruction située après un `\RETURN` garanti (même à 
    l'intérieur de blocs `\IF \ELSE`).

* **Nettoyage Mémoire :** Libération systématique (`free`) des nœuds de l'Arbre
    - Syntaxique (AST) dès qu'ils sont optimisés ou désalloués.


## Guide d'Utilisation & Compilation

Le projet utilise un `Makefile` structuré pour faciliter la compilation et
les tests. L'affichage de l'Arbre Syntaxique (AST) est enrichi par des 
codes couleurs ANSI.

### 1. Compilation du Compilateur
* `make` : Compile l'intégralité du projet et génère l'exécutable `compil`.
* `make clean` : Nettoie les fichiers temporaires et les exécutables.

### 2. Détail des étapes de compilation
Le projet décompose le processus en trois étapes distinctes si l'on souhaite
observer les fichiers intermédiaires :
* **`make make1`** : Lance le compilateur sur `test.tex`.
    - Affiche l'**Arbre Syntaxique (AST) en couleur** sur la sortie standard.
    - Génère le fichier assembleur `out.asm`.
* **`make make2`** : Appelle l'assembleur `asipro` pour transformer `out.asm`
en binaire `out.sipro`.
* **`make make3`** : Lance la machine virtuelle sur le binaire produit
(`sipro out.sipro`) pour voir le résultat final de l'algorithme.

### 3. Exécution Rapide
* **`make run`** : Enchaîne automatiquement toutes les étapes ci-dessus
(Compilation -> AST -> Assemblage -> Exécution). 
C'est la commande recommandée pour tester un nouvel algorithme.

### 4. Modes d'Optimisation
L'optimisation n'est pas activée par défaut pour permettre la comparaison.

* **Mode Standard** : 
    - `make` (Génère un code fidèle à l'écriture originale).

* **Mode Optimisé** : 
    - `make OPTIMIZE=1` (ou via le flag `-DOPTIMIZE` dans le Makefile).
    - Dans ce mode, l'AST affiché par `make make1 ou 2` montrera les nœuds
    réduits (Folding), les fonctions injectées (Inlining) et les branches 
    supprimées (Pruning).
