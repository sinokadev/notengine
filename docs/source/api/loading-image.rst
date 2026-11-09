Loading Image
=============

``loading_image.h`` is a header file that contains a hard-coded image to be displayed during loading.

The ``loadingPngBase64()`` function returns an image in the form of a base64-encoded string(``std::string``). Although the data stored internally is compressed, the returned result is automatically decompressed.

.. literalinclude:: ../../../include/knot/loading_image.h
   :language: cpp
   :caption: include/knot/loading_image.h