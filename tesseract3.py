import ctypes
import ctypes.util
import os
import logging
from multiprocessing.pool import ThreadPool
from time import sleep

TESSERACT3_LIBNAME = 'libtesseract-3.dll'
AUTOMATIC_PAGE_SEGMENTATION = 3
RAW_LINE_PAGE_SEGMENTATION = 13
logger = logging.getLogger()


class TesseractError(Exception):
    """Tesseract error class for exceptions."""
    pass


class TesseractLib(object):
    """Class for working with Tesseract 3 library and API."""

    lib = None
    api = None

    class TessBaseAPI(ctypes._Pointer):
        """Tesseract API class."""
        _type_ = type('_TessBaseAPI', (ctypes.Structure,), {})

    def setup_lib(self, lib_path=None):
        """Setup library's functions and signatures.
        API Source: https://github.com/tesseract-ocr/tesseract/blob/3.05/api/capi.h

        :param lib_path: path to Tesseract 3 library.
        """
        if self.lib is not None:
            return
        self.lib = lib = ctypes.CDLL(lib_path)

        lib.TessBaseAPICreate.restype = self.TessBaseAPI

        lib.TessBaseAPIDelete.argtypes = (self.TessBaseAPI,)
        lib.TessBaseAPIDelete.restype = None

        lib.TessBaseAPIClear.argtypes = (self.TessBaseAPI,)
        lib.TessBaseAPIClear.restype = None

        lib.TessBaseAPIEnd.argtypes = (self.TessBaseAPI,)
        lib.TessBaseAPIEnd.restype = None

        lib.TessBaseAPIInit3.argtypes = (self.TessBaseAPI, ctypes.c_char_p, ctypes.c_char_p)

        lib.TessBaseAPISetImage.argtypes = (self.TessBaseAPI,
                                            ctypes.c_void_p, ctypes.c_int, ctypes.c_int, ctypes.c_int, ctypes.c_int)
        lib.TessBaseAPISetImage.restype = None

        lib.TessBaseAPISetVariable.argtypes = (self.TessBaseAPI, ctypes.c_char_p, ctypes.c_char_p)
        lib.TessBaseAPISetVariable.restype = ctypes.c_bool

        lib.TessBaseAPIClearAdaptiveClassifier.argtypes = (self.TessBaseAPI,)
        lib.TessBaseAPIClearAdaptiveClassifier.restype = None

        lib.TessBaseAPIGetUTF8Text.restype = ctypes.c_char_p
        lib.TessBaseAPIGetUTF8Text.argtypes = (self.TessBaseAPI,)

    def __init__(self, lib_path, data_path, language="eng"):
        """Class initialization.

        :param lib_path: path to Tesseract 3 library.
        :param data_path: path to Tesseract data folder.
        :param language: OCR language.
        """
        self.setup_lib(lib_path)
        self.api = self.lib.TessBaseAPICreate()
        if self.lib.TessBaseAPIInit3(self.api, data_path.encode(), language.encode()):
            raise TesseractError('Tesseract API initialization failed.')
        self._check_setup()

    def __del__(self):
        """Library's destructor."""
        if not self.lib or not self.api:
            return
        self.lib.TessBaseAPIClear(self.api)
        self.lib.TessBaseAPIEnd(self.api)
        self.lib.TessBaseAPIDelete(self.api)

    def _check_setup(self):
        """Check if library was set up correctly."""
        if not self.lib:
            raise TesseractError('Tesseract library is not configured')
        if not self.api:
            raise TesseractError('Tesseract API is not created')

    def set_image(self, imagedata, width, height, bytes_per_pixel):
        """Set image for recognition.

        :param imagedata: image's data ctypes.
        :param width: image's width.
        :param height: image's height.
        :param bytes_per_pixel: image's depth.
        """
        self._check_setup()
        bytes_per_line = width * bytes_per_pixel
        self.lib.TessBaseAPISetImage(self.api, imagedata, width, height, bytes_per_pixel, bytes_per_line)

    def set_variable(self, key, val):
        """Set variable for library's parameter.

        :param key: parameter's key.
        :param val: parameter's value.
        """
        self._check_setup()
        self.lib.TessBaseAPISetVariable(self.api, key.encode(), val.encode())

    def get_utf8_text(self):
        """Returns UTF-8 text from image."""
        self._check_setup()
        result = self.lib.TessBaseAPIGetUTF8Text(self.api)
        self.lib.TessBaseAPIClear(self.api)
        self.lib.TessBaseAPIClearAdaptiveClassifier(self.api)
        return result

    def get_text(self):
        """Returns decoded stripped text from image."""
        self._check_setup()
        result = self.get_utf8_text()
        if result:
            return result.decode('utf-8').strip()
        return ""


class Tesseract(TesseractLib):
    """Class for working with Tesseract OCR."""

    def __init__(self, lib_path, data_path, language):
        """Class initialization.

        :param lib_path: path to Tesseract 3 library.
        :param data_path: path to Tesseract data folder.
        :param language: OCR language.
        """
        super().__init__(lib_path=lib_path, data_path=data_path, language=language)
        self.set_variable("load_system_dawg", "0")
        self.set_variable("load_freq_dawg", "0")
        self.set_variable("tessedit_oem_mode", "3")
        self.set_variable("debug_file", "/dev/null")
        self.locked = False

    def _set_default_params(self):
        """Set default parameters for recognition."""
        self.set_whitelist(whitelist="")
        self.set_psm(page_segmentation=AUTOMATIC_PAGE_SEGMENTATION)

    def set_whitelist(self, whitelist=None):
        """Set whitelist characters for recognition."""
        if whitelist is not None:
            assert isinstance(whitelist, str)
            self.set_variable("tessedit_char_whitelist", whitelist)

    def set_psm(self, page_segmentation=None):
        """Set page segmentation mode for recognition."""
        if isinstance(page_segmentation, int):
            page_segmentation = str(page_segmentation)
        if page_segmentation is not None:
            assert isinstance(page_segmentation, str)
            self.set_variable("tessedit_pageseg_mode", page_segmentation)

    def image_to_string(self, image, whitelist="", page_segmentation=AUTOMATIC_PAGE_SEGMENTATION):
        """Retrieve text from image.

        :param image: image.
        :param whitelist: whitelist characters.
        :param page_segmentation: page segmentation mode.
        """
        self.locked = True
        height, width, depth = 0, 0, 1
        if len(image.shape) == 2:
            height, width = image.shape
        elif len(image.shape) == 3:
            height, width, depth = image.shape
        self.set_whitelist(whitelist=whitelist)
        self.set_psm(page_segmentation=page_segmentation)
        self.set_image(imagedata=image.ctypes, width=width, height=height, bytes_per_pixel=depth)
        result = self.get_text()
        self._set_default_params()
        self.locked = False
        return result


class TesseractPool:
    """Class for working with multiple instances of Tesseract."""

    @staticmethod
    def find_tesseract_lib():
        """Find Tesseract library in PATH, WORKDIR, etc."""
        lib_path = ctypes.util.find_library(TESSERACT3_LIBNAME)
        if lib_path is None:
            raise TesseractError('Tesseract library is not found')
        return lib_path

    def __init__(self, processes=None, language='eng', data_folder='tessdata'):
        """Class initialization.

        :param processes: number of Tesseract processes to create.
        :param language: OCR language.
        :param data_folder: name of Tesseract data folder.
        """
        if processes is None:
            processes = os.cpu_count()
        self.language = language
        self.lib_path = self.find_tesseract_lib()
        lib_folder = os.path.dirname(self.lib_path)
        self.data_path = os.path.join(lib_folder, data_folder)
        init_params = [(self.lib_path, self.data_path, self.language) for _ in range(processes)]
        with ThreadPool() as pool:
            logger.debug(f"Creating {processes} Tesseract API(s) with '{language}' language.")

            def init_tesseract_instance(*args, **kwargs):
                return Tesseract(*args, **kwargs)
            self._pool = pool.starmap(init_tesseract_instance, init_params)

    def image_to_string(self, image, whitelist=None, page_segmentation=3):
        """Retrieve text from image from available Tesseract instance.

        :param image: image.
        :param whitelist: whitelist characters.
        :param page_segmentation: page segmentation mode.
        """
        non_locked = [tess for tess in self._pool if not tess.locked]
        if not non_locked:
            sleep(0.05)
            return self.image_to_string(image=image, whitelist=whitelist, page_segmentation=page_segmentation)
        return non_locked[0].image_to_string(image=image, whitelist=whitelist, page_segmentation=page_segmentation)
