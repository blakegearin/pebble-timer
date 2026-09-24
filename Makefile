# Screenshot chores. The work itself lives in tools/screenshots.sh; this is
# the loop a human would mistype: which scene belongs to which platform,
# where the README shots live, and that the tour starts from a wiped state.
#
## Targets
##   make shots             refresh assets/screenshots/ for every platform
##   make shot-<platform>   ...for one platform (e.g. make shot-chalk)
##   make check             diff the empty menu against the upstream baselines
##
## aplite has no cog row -- its settings are inline rows -- so its tour is a
## separate scene with three shots rather than four.

PLATFORMS := aplite basalt chalk diorite emery gabbro
BASELINED := aplite basalt chalk
ASSETS    := assets/screenshots
BASEDIR   := tmp/baselines/upstream-master
TOUR      := tools/scenes/readme-tour.scene
TOUR_BW   := tools/scenes/readme-aplite.scene
EMPTY     := tools/scenes/menu-empty.scene
SHOTS     := tools/screenshots.sh

.NOTPARALLEL:

.PHONY: help shots check $(PLATFORMS:%=shot-%)

help:
	@sed -n '/^## /s/^## //p' Makefile

shots: $(PLATFORMS:%=shot-%)

# An explicit list, not a shot-% pattern rule: macOS ships GNU make 3.81,
# which silently skips pattern rules once .PHONY has created empty entries
# for the same names.
$(PLATFORMS:%=shot-%):
	@plat=$(@:shot-%=%); \
	scene=$(TOUR); \
	[ "$$plat" = aplite ] && scene=$(TOUR_BW); \
	$(SHOTS) -p $$plat -w -o $(ASSETS)/$$plat $$scene

check:
	@for plat in $(BASELINED); do \
		$(SHOTS) -p $$plat -w -b $(BASEDIR)/$$plat \
			-o tmp/shots/check/$$plat $(EMPTY) || exit 1; \
	done
