from ctypes import windll
from PIL import Image, ImageGrab
from numpy import array
from pycaw.pycaw import AudioUtilities
import win32gui, win32ui, win32api, win32process, win32con
import pywintypes
import time
import cv2
from tesseract3 import TesseractPool, AUTOMATIC_PAGE_SEGMENTATION
from string import ascii_letters


PW_RENDERFULLCONTENT = 2  # Properly capture DirectComposition window contents. Available from Windows 8.1
tesseract = TesseractPool(language="eng", processes=1)


def set_low_cpu_priority():
    """Set low CPU priority for current process."""
    pid = win32api.GetCurrentProcessId()
    handle = win32api.OpenProcess(win32con.PROCESS_ALL_ACCESS, True, pid)
    win32process.SetPriorityClass(handle, win32process.BELOW_NORMAL_PRIORITY_CLASS)


def get_text_from_image(image_gray, whitelist):
    """Get text from image using Tesseract OCR.
    https://github.com/tesseract-ocr/

    :param image_gray: numpy.array of image.
    :param whitelist: available character in image's text.

    :return: text from image.
    """
    return tesseract.image_to_string(image_gray, whitelist=whitelist, page_segmentation=AUTOMATIC_PAGE_SEGMENTATION)


def is_strings_similar(original, compare, overlap=0.4):
    """Check if strings are similar.

    :param original: original string.
    :param compare: string to compare.
    :param overlap: overlap parameter. If string's similarity >= overlap then strings are similar.

    :return: True or False.
    """
    def levenshtein_distance(a, b):
        """Return the Levenshtein edit distance between two strings."""
        if a == b:
            return 0
        if len(a) < len(b):
            a, b = b, a
        prev_row = range(len(b) + 1)
        for i, column1 in enumerate(a):
            cur_row = [i + 1]
            for j, column2 in enumerate(b):
                insertions = prev_row[j + 1] + 1
                deletions = cur_row[j] + 1
                substitutions = prev_row[j] + (column1 != column2)
                cur_row.append(min(insertions, deletions, substitutions))
            prev_row = cur_row
        return prev_row[-1]

    distance = levenshtein_distance(original.upper(), compare.upper())
    non_similarity = distance / len(original) if len(original) > 0 else 1
    return non_similarity <= overlap


class GenshinImpact:
    """Class for working with Genshin Impact."""

    GENSHIN_IMPACT_EXE = "GenshinImpact.exe"
    PAIMON_NAME_COLOR_RANGE = ((230, 170, 0), (255, 210, 10))
    PAIMON_DEFAULT_DIALOGUE = (0.4631496915663028, 0.7872131016037641, 0.536658108769003, 0.8286028539255994)
    PAIMON_OUT_DIALOGUE = (0.4645968857034299, 0.7582338162046451, 0.5333868470215386, 0.8020998784944536)
    PAIMON_NAME = "Paimon"
    DIALOGUE_NAME_CHARACTERS = "".join(set(ascii_letters))

    def __init__(self, window_name="Genshin Impact", window_class="UnityWndClass"):
        self.window_name = window_name
        self.window_class = window_class
        self.window = None
        self.screen_locked = False
        self.last_frame = None
        self.window_active = False
        self.position_box = None
        self._find_window_if_necessary()
        self.full_width = win32api.GetSystemMetrics(0)
        self.full_height = win32api.GetSystemMetrics(1)

    @property
    def is_full_screen(self):
        if self.window and self.window_active:
            foreground = win32gui.GetForegroundWindow() == self.window
            top_most_style = win32gui.GetWindowLong(self.window, win32con.GWL_EXSTYLE) & win32con.WS_EX_TOPMOST
            return foreground and top_most_style == win32con.WS_EX_TOPMOST

    def _find_window(self, hwnd, wildcard):
        """Enumerates over windows to find Genshin Impact process and store info about it."""
        try:
            if "Genshin Impact" == win32gui.GetWindowText(hwnd) and "UnityWndClass" == win32gui.GetClassName(hwnd):
                self.window = hwnd
                _, _, _, _, self.position_box = win32gui.GetWindowPlacement(hwnd)
                self.width = self.position_box[2] - self.position_box[0]
                self.height = self.position_box[3] - self.position_box[1]
        except pywintypes.error:
            pass

    def _find_window_if_necessary(self):
        if not self.window or (not win32gui.GetWindowText(self.window) == self.window_name):
            win32gui.EnumWindows(self._find_window, None)
            if win32gui.GetWindowText(self.window) == self.window_name:
                if not self.window_active:
                    self.window_active = True
                    print("Ready for Paimon!")
            else:
                if self.window_active:
                    print("Game was closed.")
                    self.window_active = False
                    self.window = None

    def _get_window_image(self, width, height):
        """Gets Genshin Impact window's image.

        :param width: frame's width.
        :param height: frame's height.
        :return: PIL.Image image.
        """
        while self.screen_locked:
            if self.last_frame:
                return self.last_frame
            time.sleep(0.01)
        self.screen_locked = True

        hwnd_dc = win32gui.GetWindowDC(self.window)
        mfc_dc = win32ui.CreateDCFromHandle(hwnd_dc)
        save_dc = mfc_dc.CreateCompatibleDC()

        bit_map = win32ui.CreateBitmap()
        bit_map.CreateCompatibleBitmap(mfc_dc, width, height)

        save_dc.SelectObject(bit_map)
        windll.user32.PrintWindow(self.window, save_dc.GetSafeHdc(), PW_RENDERFULLCONTENT)

        bmp_info = bit_map.GetInfo()
        bmp_arr = bit_map.GetBitmapBits(True)

        win32gui.DeleteObject(bit_map.GetHandle())
        save_dc.DeleteDC()
        mfc_dc.DeleteDC()
        win32gui.ReleaseDC(self.window, hwnd_dc)

        self.screen_locked = False
        img = Image.frombuffer('RGB', (bmp_info['bmWidth'], bmp_info['bmHeight']), bmp_arr, 'raw', 'BGRX', 0, 1)
        self.last_frame = img
        return img

    def _get_all_screen(self):
        """Gets fullscreen image.

        :return: PIL.Image image.
        """
        while self.screen_locked:
            if self.last_frame:
                return self.last_frame
            time.sleep(0.01)
        self.screen_locked = True
        _, _, _, _, self.position_box = win32gui.GetWindowPlacement(self.window)
        img = ImageGrab.grab(bbox=self.position_box, all_screens=True)
        self.screen_locked = False
        self.last_frame = img
        return img

    def _get_frame(self):
        if self.is_full_screen:
            return self._get_all_screen()
        else:
            return self._get_window_image(self.width, self.height)

    def _get_text_from_rect(self, frame, rect):
        """Gets text from frame by the rectangle (to support any available resolution).

        :param rect: rectangle.
        :return: text from tesseract.
        """
        try:
            box = (rect[0] * frame.width, rect[1] * frame.height,
                   rect[2] * frame.width, rect[3] * frame.height)
            frame = frame.crop(box)
            color_low, color_high = array(self.PAIMON_NAME_COLOR_RANGE[0]), array(self.PAIMON_NAME_COLOR_RANGE[1])
            array_image = cv2.inRange(array(frame), color_low, color_high)
            text = get_text_from_image(image_gray=array_image, whitelist=self.DIALOGUE_NAME_CHARACTERS)
            return text
        except BaseException as err:
            print(err)
            return ""

    def _set_mute(self, mute=False):
        """Set mute for Genshin Impact process."""
        sessions = AudioUtilities.GetAllSessions()
        for session in sessions:
            if session.Process and session.Process.name() == self.GENSHIN_IMPACT_EXE:
                volume = session.SimpleAudioVolume
                volume.SetMute(mute, None)

    def _is_paimon_speaking(self):
        """Returns when Paimon's name on the screen of dialogue."""
        frame = self._get_frame()
        in_dialogue = is_strings_similar(self._get_text_from_rect(frame=frame, rect=self.PAIMON_DEFAULT_DIALOGUE),
                                         self.PAIMON_NAME)
        out_dialogue = is_strings_similar(self._get_text_from_rect(frame=frame, rect=self.PAIMON_OUT_DIALOGUE),
                                          self.PAIMON_NAME)
        return in_dialogue or out_dialogue

    def paimon_shut_up(self):
        """Check for Paimon's dialogue and shut her up."""
        paimon_was_here = False
        try:
            while True:
                self._find_window_if_necessary()
                if not self.window_active:
                    continue
                is_paimon_speaking = self._is_paimon_speaking()
                if is_paimon_speaking and not paimon_was_here:
                    paimon_was_here = True
                    print("Paimon, shut up!")
                    self._set_mute(is_paimon_speaking)
                if not is_paimon_speaking and paimon_was_here:
                    paimon_was_here = False
                    print("Unmuting the game.")
                    self._set_mute(is_paimon_speaking)
        except KeyboardInterrupt:
            self._set_mute(False)


if __name__ == '__main__':
    set_low_cpu_priority()
    print("Waiting for GenshinImpact.exe process")
    game = GenshinImpact()
    game.paimon_shut_up()
