Scene asset paths
=================

``resolveSceneAssetPath`` resolves asset references found in scene files.
Relative paths are interpreted from the scene file's directory. Paths that
contain ``{sourceRoot}`` have that token replaced by the configured game source
root. The legacy ``{assetRoot}`` token remains supported. Absolute paths pass
through unchanged.

.. doxygenfunction:: knot::resolveSceneAssetPath
