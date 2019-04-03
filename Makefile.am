SUBDIRS = src

dist_doc_DATA = README COPYING doc/messages.md doc/phantom_howto.md

install-data-hook:
	$(MKDIR_P) $(DESTDIR)$(mandir)/man1
	gzip -cf9 doc/dimple.1 > $(DESTDIR)$(mandir)/man1/dimple.1.gz

EXTRA_DIST = autogen.sh bootstrap.sh

EXTRA_DIST += test/balljoint.sh test/collide.sh test/cube.sh				\
	test/cylinder.3ds test/cylinder.sh test/destroy.sh test/fixed.sh	\
	test/free.sh test/grab.sh test/gravity.sh test/hinge.sh						\
	test/hinge2.sh test/manyspheres.sh test/piston.sh test/slide.sh		\
	test/springhinge.sh test/test.pd test/texture.sh									\
	test/therasphere.ck test/universal.sh test/wall.pd

EXTRA_DIST += maxmsp/test.maxpat maxmsp/wall.maxpat maxmsp/README.rtf	\
	maxmsp/balljoint.maxpat maxmsp/collide.maxpat maxmsp/cube.maxpat		\
	maxmsp/cylinder.3ds maxmsp/cylinder.maxpat maxmsp/destroy.maxpat		\
	maxmsp/dimple-io.maxpat maxmsp/fixed.maxpat maxmsp/free.maxpat			\
	maxmsp/grab.maxpat maxmsp/gravity.maxpat maxmsp/hinge.maxpat				\
	maxmsp/hinge2.maxpat maxmsp/manyspheres.maxpat maxmsp/piston.maxpat	\
	maxmsp/slide.maxpat maxmsp/springhinge.maxpat												\
	maxmsp/universal.maxpat maxmsp/supplemental/README.rtf							\
	maxmsp/supplemental/query_cursor.maxpat															\
	maxmsp/supplemental/stiffness.maxpat

EXTRA_DIST += icon/dimple.png icon/dimple.rc.in			\
	icon/dimple_sphere.blend icon/dimple_sphere.icns	\
	icon/dimple_sphere.ico icon/dimple_sphere.png

EXTRA_DIST += textures/bumpy.png textures/grainy.png	\
	textures/marble.png
