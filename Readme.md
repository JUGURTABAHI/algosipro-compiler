# AlgoSipro Compiler

Compilateur développé en **C**, avec **Flex** et **Bison**, traduisant des
algorithmes écrits dans une syntaxe inspirée de LaTeX vers l’assembleur
de la machine virtuelle **Sipro**.

Le projet comprend l’analyse lexicale et syntaxique, la construction d’un
arbre syntaxique abstrait (AST), des vérifications de types, des optimisations
optionnelles et la génération de code assembleur.

## Contexte

Projet réalisé dans le cadre de l’UE Compilation, en troisième année
de licence Informatique à l’Université de Rouen Normandie.

- **Auteur :** Jugurta BAHI
- **Encadrant :** Arnaud LEFEBVRE

## Fonctionnalités

- Définition de plusieurs algorithmes dans un même fichier.
- Variables locales et paramètres de fonctions.
- Expressions arithmétiques et comparaisons.
- Types `INT` et `BOOL`.
- Conditions avec ou sans branche alternative.
- Boucles `FOR` et `WHILE`.
- Appels de fonctions et récursivité directe.
- Affichage d’entiers, de booléens et de chaînes.
- Construction et affichage de l’AST.
- Optimisations optionnelles de l’AST.
- Génération d’assembleur pour Sipro.

## Prérequis

Les outils suivants doivent être installés et accessibles dans le `PATH` :

- GCC compatible avec les fonctionnalités C utilisées par le projet,
  notamment `nullptr`, avec l’option `-std=c2x` ;
- Flex ;
- Bison ;
- Make ;
- `asipro`, pour assembler le code généré ;
- `sipro`, pour exécuter le programme assemblé.

## Compilation et exécution

### Compiler le compilateur

```sh
make
```

Cette commande génère l’exécutable `compil`.

### Exécuter le programme de test

```sh
make run
```

Par défaut, le fichier d’entrée est `test.tex`.

La commande enchaîne les étapes suivantes :

1. Compilation du programme AlgoSipro vers `out.asm`.
2. Assemblage avec `asipro` vers `out.sipro`.
3. Exécution avec `sipro`.


### Exécuter les étapes séparément

| Commande | Action |
|----------|--------|
| `make make1` | Génère `out.asm` à partir du fichier d’entrée. |
| `make make2` | Effectue l’étape précédente, puis génère `out.sipro`. |
| `make make3` | Effectue les étapes précédentes, puis exécute le programme. |
| `make run` | Lance la chaîne complète, comme `make make3`. |
| `make clean` | Supprime les fichiers de construction prévus par la cible de nettoyage. |

L’AST est affiché sur la **sortie d’erreur standard (`stderr`)**.
L’assembleur est écrit sur la **sortie standard (`stdout`)**, redirigée
vers `out.asm` par le Makefile.

Les fichiers `out.asm` et `out.sipro` sont des fichiers générés :
ils ne sont pas les sources du projet.

## Exemple de programme

```tex
\begin{algo}{doubleval}{n}
  \RETURN{n + n}
\end{algo}

\begin{algo}{main}{}
  \SET{x}{\CALL{doubleval}{6}}
  \PRINTS{"Resultat : "}
  \PRINTD{x}
  \PRINTS{"\n"}
  \RETURN{0}
\end{algo}

\CALL{main}{}
```

Résultat :

```text
Resultat : 12
0
```

Le dernier `0` correspond à la valeur renvoyée par l’appel final,
automatiquement affichée par le compilateur.

## Syntaxe du langage

### Définition d’un algorithme

```tex
\begin{algo}{nom}{param1, param2}
  ...
  \RETURN{expression}
\end{algo}
```

Un algorithme peut avoir zéro, un ou plusieurs paramètres.

Le compilateur exige un retour garanti selon son analyse structurelle :
par exemple, un `\RETURN` final ou un `\IF` dont les deux branches
garantissent un retour.

### Variables et affectations

```tex
\SET{x}{10}
\SET{x}{x + 3}
```

Les variables sont locales à l’algorithme qui les contient.
Leur type est déterminé lors de leur première affectation et ne peut
pas être changé par une affectation ultérieure.

### Conditions

```tex
\IF{x > 0}
  \PRINTS{"positif\n"}
\ELSE
  \PRINTS{"nul ou negatif\n"}
\FI
```

La branche `\ELSE` est optionnelle.
La condition doit être de type `BOOL`.

### Boucle WHILE

```tex
\SET{i}{1}

\DOWHILE{i <= 5}
  \PRINTD{i}
  \PRINTS{"\n"}
  \SET{i}{i + 1}
\OD
```

La condition est évaluée avant chaque itération.
Malgré son nom, `\DOWHILE` correspond donc à une boucle **tant que**,
et non à une boucle dont le test est effectué à la fin.

### Boucle FOR

```tex
\DOFORI{i}{1}{5}
  \PRINTD{i}
  \PRINTS{"\n"}
\OD
```

La boucle parcourt les valeurs de la borne inférieure à la borne
supérieure **incluse**, avec un incrément de `1`.

Les bornes peuvent être des expressions entières.
Dans la génération actuelle, elles sont évaluées à l’entrée de la boucle.

### Appels de fonctions

Un appel peut être utilisé dans une expression :

```tex
\SET{x}{\CALL{doubleval}{6}}
\PRINTD{\CALL{doubleval}{8}}
```

Le nombre d’arguments doit correspondre au nombre de paramètres.
Les arguments doivent être de type `INT`.

Une fonction appelée doit déjà être connue du compilateur.
La récursivité directe est prise en charge, mais les appels vers des
fonctions définies plus loin dans le fichier ne sont pas pris en charge.

### Point d’entrée

Le fichier doit se terminer par un appel :

```tex
\CALL{main}{}
```

Cet appel déclenche l’exécution du programme.
Sa valeur de retour est affichée, suivie d’un saut de ligne.

### Affichage

```tex
\PRINTD{expression}
\PRINTS{"texte"}
```

- `\PRINTD` affiche un entier ou un booléen.
- Les booléens sont affichés sous la forme `True` ou `False`.
- `\PRINTS` affiche une chaîne.
- Aucun saut de ligne n’est ajouté automatiquement par ces instructions.

Pour ajouter un saut de ligne :

```tex
\PRINTS{"\n"}
```

## Types et expressions

### Types

| Type | Utilisation |
|------|-------------|
| `INT` | Valeurs entières, calculs arithmétiques et paramètres. |
| `BOOL` | Résultats des comparaisons et conditions. |

Les fonctions peuvent produire un résultat de type `INT` ou `BOOL`.

Les opérations arithmétiques exigent des opérandes de type `INT`.
Les conditions de `IF` et de `WHILE` exigent un `BOOL`.
Une affectation ne convertit pas implicitement un entier en booléen,
ni un booléen en entier.

### Opérateurs

| Catégorie | Opérateurs |
|-----------|------------|
| Arithmétique | `+`, `-`, `*`, `/` |
| Comparaison | `<`, `>`, `<=`, `>=`, `==`, `!=` |
| Parenthésage | `(` et `)` |

Les multiplications et divisions sont prioritaires sur les additions
et soustractions. Les parenthèses permettent de modifier cet ordre.

Les opérateurs logiques `&&` et `||` ne sont pas implémentés dans
la grammaire actuelle.

Le moins unaire n’est pas pris en charge : une valeur négative peut
être exprimée par une soustraction, par exemple `0 - 5`.

## Optimisations

Les optimisations AST sont activées par la macro `OPTIMIZE`.

Certaines transformations sont réalisées pendant la construction de
l’AST, puis une passe de réduction est répétée jusqu’à stabilisation.

### Activer les optimisations

Avec le Makefile fourni :

```sh
make clean
make run OPT=-DOPTIMIZE
```

### Exécuter sans optimisation AST

Avec les options par défaut du Makefile :

```sh
make clean
make run
```

La définition `-DxOPTIMIZE` n’active pas les blocs `#ifdef OPTIMIZE` :
`xOPTIMIZE` et `OPTIMIZE` sont deux noms de macros différents.

Un nettoyage est nécessaire lors du changement de mode pour garantir
la reconstruction des fichiers objets avec les bonnes options.

### Évaluation des expressions constantes

Les expressions dont les opérandes sont constants peuvent être calculées
à la compilation.

Exemple :

```text
2 + 3 * 4  →  14
```

Cette réduction concerne les opérations arithmétiques et les comparaisons.
Les comparaisons réduites conservent leur type `BOOL`.

### Propagation de constantes

Certaines références à des variables identifiées comme constantes
sont remplacées par leur valeur.

L’implémentation actuelle repose notamment sur le nombre d’affectations
enregistrées pour chaque variable. Il ne s’agit pas d’une analyse complète
du flot de données.

### Simplifications algébriques

Les transformations implémentées comprennent notamment :

```text
x + 0  →  x
0 + x  →  x
x - 0  →  x
x * 1  →  x
1 * x  →  x
x / 1  →  x
x - x  →  0
```

La simplification `x - x` concerne des références au même identifiant,
pas deux appels de fonction.

Certaines constantes sont également regroupées :

```text
(x + 5) + 10  →  x + 15
```

Les expressions `appel() * 0` et `0 * appel()` conservent l’évaluation
de l’appel et ses effets.

Les expressions `x / x` et `0 / x` ne sont pas simplifiées
systématiquement : le dénominateur peut être nul.

### Inlining limité

Un appel peut être remplacé par une copie de l’expression retournée
lorsque :

- la fonction est constituée d’une unique instruction `RETURN` ;
- tous les arguments sont des valeurs constantes au moment
  de cette transformation ;
- le nombre d’arguments correspond au nombre de paramètres.

Cette restriction évite notamment de supprimer ou de dupliquer
l’évaluation d’arguments contenant des appels.

### Simplification du contrôle

Les transformations comprennent :

- la sélection de la branche active d’un `IF` constant ;
- la suppression d’un `WHILE` dont la condition est constamment fausse ;
- la suppression de certains `FOR` dont les bornes constantes
  ne permettent aucune itération.

Le compilateur élimine aussi des instructions situées après un retour
garanti. Cette dernière transformation est effectuée lors de la liaison
des instructions, y compris sans la macro `OPTIMIZE`.

### Divisions constantes invalides

Lors de l’évaluation d’une division entièrement constante,
un dénominateur nul provoque une erreur de compilation.

Ce contrôle ne constitue pas une détection générale de toutes
les divisions par zéro possibles à l’exécution.

## Tests

Le fichier `test.tex` regroupe des tests de régression portant sur :

- les calculs et les priorités arithmétiques ;
- la propagation de constantes et les réaffectations ;
- les comparaisons et l’affichage des booléens ;
- les conditions simples et imbriquées ;
- les boucles `FOR`, `WHILE` et les boucles imbriquées ;
- la conservation des effets des appels ;
- les arguments utilisés plusieurs fois ou ignorés ;
- la récursivité ;
- la conservation des variables locales et des résultats entre appels.

Le programme de test a été exécuté avec et sans optimisation.
Les sorties observées sont identiques pour ces cas.

L’AST peut différer entre les deux modes : l’objectif est de transformer
le programme sans modifier son comportement observable.

Ces tests ne constituent pas une preuve de correction pour tous
les programmes possibles.

## Organisation du projet

| Fichier | Rôle |
|---------|------|
| `compil.l` | Analyseur lexical Flex. |
| `compil.y` | Grammaire Bison et actions associées. |
| `AST.c`, `AST.h` | Construction, transformation et génération de code de l’AST. |
| `hashtable.c`, `hashtable.h` | Tables de symboles et informations sur les variables et fonctions. |
| `commun.c`, `commun.h` | Définitions et fonctions partagées. |
| `Makefile` | Construction et chaîne d’exécution. |
| `test.tex` | Programme de test de régression. |
| `.gitignore` | Exclusion des fichiers générés du suivi Git. |

## Portée du projet

Ce compilateur est un projet pédagogique.

Les optimisations et les vérifications couvrent les fonctionnalités
décrites ci-dessus, avec les limites indiquées. La gestion des cas limites,
notamment les débordements arithmétiques et les analyses avancées du
flot de contrôle, ne doit pas être considérée comme exhaustive.

## Attribution

L’implémentation de la table de hachage utilisée dans ce projet est
issue des travaux de M. Christophe HANCART, professeur des universités
à l’Université de Rouen Normandie.

