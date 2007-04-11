
dimple:
	python libdeps/scons.py -C src

debug:
	python libdeps/scons.py -C src debug=1

clean:
	python libdeps/scons.py -C src -c
