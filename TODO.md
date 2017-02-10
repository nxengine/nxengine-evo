* Add i18n support via gettext
* make paths configurable
  * store saves/settings in user dir based on a platform
  * load data from platform-specific dir (e.g. /usr/share on linux)
  * move all custom files to data/
  * Add runtime language switching support
* drop out sif and use original data
* make compatible with nicalis version
* use wavs for drums?
* rewrite soundlib to use SDL_mixer
