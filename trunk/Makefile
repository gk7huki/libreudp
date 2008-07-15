EXAMPLE_FILES=examples/*.cpp examples/README
DIST_TAR_EXCLUDES=--exclude='.*' --exclude=makefile --exclude='*.exe'
DIST_BASENAME=libreudp
DIST_FILES=LICENSE README Makefile SConstruct build_config.py.dist build_support.py src build

default: release

clean: clean_release

all: release

test: all
	scons test
	
clean_all: clean_release clean_debug

release: build_release

debug: build_debug

build_release:
	scons

build_debug:
	scons debug=yes

clean_release:
	scons -c
	scons -c test example

clean_debug:
	scons -c debug=yes
	scons -c debug=yes test example

dist: clean_all
	mkdir -p dist_tmp/$(DIST_BASENAME) 
	cp -R $(DIST_FILES) dist_tmp/$(DIST_BASENAME)
	cd dist_tmp;tar $(DIST_TAR_EXCLUDES) -cvf ../$(DIST_BASENAME).tar $(DIST_BASENAME)
	gzip $(DIST_BASENAME).tar
	rm -rf dist_tmp

