"""
Non-blocking update checker using the GitHub Releases API.
Compares the local VERSION against the latest published release.
Reusable across all Bitemu editions — only GITHUB_REPO needs changing.
"""

import json
import re

from PySide6.QtCore import QObject, QUrl, Signal
from PySide6.QtNetwork import QNetworkAccessManager, QNetworkRequest, QNetworkReply

GITHUB_REPO = "IRodriguez13/Bitemu"
API_URL = f"https://api.github.com/repos/{GITHUB_REPO}/releases/latest"


def parse_version(tag: str) -> tuple[int, ...]:
    """Extract numeric parts from a version string like 'v0.2.1-beta' -> (0, 2, 1)."""
    nums = re.findall(r"\d+", tag)
    return tuple(int(n) for n in nums)


def is_newer(remote: str, local: str) -> bool:
    """True when remote version is strictly greater than local."""
    return parse_version(remote) > parse_version(local)


class UpdateChecker(QObject):
    """
    Async check for a newer GitHub release.

    Signals:
        update_available(tag: str, url: str)  — emitted if a newer release exists.
    """

    update_available = Signal(str, str)

    def __init__(self, current_version: str, parent: QObject | None = None):
        super().__init__(parent)
        self._current = current_version
        self._nam = QNetworkAccessManager(self)

    def check(self):
        """Fire-and-forget: emits update_available if a newer release is found."""
        req = QNetworkRequest(QUrl(API_URL))
        req.setRawHeader(b"Accept", b"application/vnd.github+json")
        req.setRawHeader(b"User-Agent", b"Bitemu-UpdateChecker")
        reply = self._nam.get(req)
        reply.finished.connect(lambda: self._on_finished(reply))

    def _on_finished(self, reply: QNetworkReply):
        try:
            if reply.error() != QNetworkReply.NetworkError.NoError:
                return
            data = json.loads(bytes(reply.readAll()).decode("utf-8"))
            tag = data.get("tag_name", "")
            url = data.get("html_url", "")
            if tag and url and is_newer(tag, self._current):
                self.update_available.emit(tag, url)
        finally:
            reply.deleteLater()
