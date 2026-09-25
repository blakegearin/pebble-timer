# Repo chores: the pebble tool's common flows, and the screenshot harness
# (whose work lives in tools/screenshots.sh -- this is the loop a human would
# mistype: which scene belongs to which platform, where the README shots
# live, and that the tour starts from a wiped state).
#
## Targets
##   make build               compile all six platforms into build/
##   make run [PLAT=basalt]   build and install onto an emulator (also boots it)
##   make kill                stop every running emulator
##   make clean               remove build/ (screenshots live in tmp/, ignored)
##   make shots               refresh assets/screenshots/ for every platform
##   make shot-<platform>     ...for one platform (e.g. make shot-chalk)
##   make check               diff the empty menu against the upstream baselines
##
## aplite has no cog row -- its settings are inline rows -- so its tour is a
## separate scene with three shots rather than four. The Accent Color setting
## is a third scene, run on the four colour platforms only: aplite has no
## settings windows and diorite is black and white, so it exists on neither.
PLATFORMS := aplite basalt chalk diorite emery gabbro
COLOUR    := basalt chalk emery gabbro
BASELINED := aplite basalt chalk
ASSETS    := assets/screenshots
BASEDIR   := tmp/baselines/upstream-master
TOUR      := tools/scenes/readme-tour.scene
TOUR_BW   := tools/scenes/readme-aplite.scene
COLOR     := tools/scenes/color-picker.scene
EMPTY     := tools/scenes/menu-empty.scene
SHOTS     := tools/screenshots.sh

PLAT ?= basalt

.NOTPARALLEL:

.PHONY: help build run kill clean shots check $(PLATFORMS:%=shot-%)

help:
	@sed -n '/^##/s/^## \{0,1\}//p' Makefile

build:
	pebble build

sideload:
	pebble install --phone

# Guard the target: `make run PLAT=blsa` would otherwise hand the typo to the
# pebble tool and fail at install time with a stranger error.
run:
	@if [ -z "$(filter $(PLATFORMS),$(PLAT))" ]; then \
		echo "unknown platform '$(PLAT)' -- try: $(PLATFORMS)"; exit 1; \
	fi
	pebble build && pebble install --emulator $(PLAT)

kill:
	pebble kill --force || true

clean:
	rm -rf build

shots: $(PLATFORMS:%=shot-%)

# An explicit list, not a shot-% pattern rule: macOS ships GNU make 3.81,
# which silently skips pattern rules once .PHONY has created empty entries
# for the same names.
$(PLATFORMS:%=shot-%):
	@plat=$(@:shot-%=%); \
	scene=$(TOUR); \
	[ "$$plat" = aplite ] && scene=$(TOUR_BW); \
	$(SHOTS) -p $$plat -w -o $(ASSETS)/$$plat $$scene; \
	case " $(COLOUR) " in *" $$plat "*) \
		$(SHOTS) -p $$plat -w $(COLOR); \
		cp tmp/shots/color-picker/$$plat/*-color-*.png $(ASSETS)/$$plat/;; \
	esac

check:
	@for plat in $(BASELINED); do \
		$(SHOTS) -p $$plat -w -b $(BASEDIR)/$$plat \
			-o tmp/shots/check/$$plat $(EMPTY) || exit 1; \
	done
