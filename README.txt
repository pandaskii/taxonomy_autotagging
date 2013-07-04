taxonomy-auto-tagging
=====================

Drupal module - Taxonomy Auto Tagging

Drupal Core: 7.x

CONTENTS OF THIS FILE
=====================

* Introduction
* Installation
* Configuration
* Drush

Introduction
============

Current Maintainer: Joseph Z (joseph@gotouch.com.au)

The Taxonomy Auto Tagging can automatically attach the tags with nodes,
and the whole process is based on pre-defined vocabulary terms and synonyms
associated with the node content type.

Features:

* Plural/Singular term(s) support.
* Node(s) can be excluded from the auto tagging process and display.
* Drush support: drush tat.
* Support Synonyms module.
* The matching terms information can be chosen saving into database.


Installation
============

The Taxonomy Auto Tagging module is dependent on: Taxonomy

Also optional module: Synonyms

Installation is quite simple,
Enable the module or you can use Drush: drush pm-enable taxonomy_autotagging.


Configuration
=============

1. Go to Content Type admin/structure/types.
2. Choose the Content Type which contained the Taxonomy field.
3. Go to manage fields then edit
4. Choose the Taxonomy Auto Tagging options

You can click the button "Run the taxonomy auto tagging",
then the module will automatically bulk process the nodes and attach the tag(s).


Drush
=====

drush tat - will bulk process the whole nodes in the database
if the Taxonomy Auto Tagging is set to "Enabled".
