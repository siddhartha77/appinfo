# AppInfo
Extensible Classic Mac application to get information about other applications.

# Compiling
[CodeWarrior 6](https://macintoshgarden.org/apps/codewarrior-6) required. Two files, *AppInfo.mcp.sit* and *AppInfo.rsrc.sit* will need to be expanded with [StuffIt Expander](https://macintoshgarden.org/apps/stuffit-expander-55). These are the CodeWarrior project file and resource file, respectively.

# Creating Plugins
Using [Resorcerer](https://macintoshgarden.org/apps/resorcerer-125), you can add your own plugins to AppInfo. Plugins are defined in the PLUG resource.

To organize the plugins, IDs are categorized below:

* 1000 - Language (LANG)
* 2000 - Framework (FMWK)
* 3000 - Engine (NGIN)
* 4000 - Copy Protection (COPY)

The name of the resource is what will be displayed in AppInfo.

Each plugin needs to have a category corresponding to the four character code in the list above. Add a resource type and optionally a list of resource IDs and/or names for each type. A Data Fork Identifier specifies an offset then a maximum of 255 bytes to match.

The application being analyzed needs to match ALL the specfied criteria to be a match.
