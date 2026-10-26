Scene asset paths
=================

``resolveSceneAssetPath`` resolves asset references found in scene files.
Relative paths are interpreted from the scene file's directory. Paths that
contain the ``{assetRoot}`` token have that token replaced by the configured
asset root, while absolute paths pass through unchanged.

.. doxygenfunction:: knot::resolveSceneAssetPath
