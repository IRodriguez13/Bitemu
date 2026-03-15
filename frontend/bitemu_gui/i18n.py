"""
i18n: español e inglés. Mapeo de strings para toda la UI.
"""
from PySide6.QtCore import QSettings

LANG_KEY = "bitemu/language"
DEFAULT_LANG = "es"
SUPPORTED = ("es", "en")

_TRANSLATIONS = {
    "es": {
        "splash.start": "Start",
        "splash.options": "Opciones",
        "splash.quit": "Cerrar Bitemu",
        "splash.credit": "Created by: Iván Ezequiel Rodriguez",
        "lobby.title": "Elegí una edición",
        "lobby.back": "← Volver",
        "lobby.soon": "Próximamente",
        "library.header": "Game Selection",
        "library.back": "← Volver",
        "library.open_rom": "Abrir ROM...",
        "library.rom_folder": "Carpeta de ROMs...",
        "library.search": "Buscar...",
        "library.empty": "No hay ROMs. Elegí una carpeta de ROMs para comenzar.",
        "library.select_game": "Seleccioná un juego",
        "library.load_rom": "Cargar ROM",
        "footer.back": "Volver",
        "footer.load": "Cargar ROM",
        "menu.file": "&Archivo",
        "menu.open_rom": "Abrir ROM...",
        "menu.recent": "ROMs recientes",
        "menu.none": "(ninguna)",
        "menu.quit": "Salir",
        "menu.emulation": "&Emulación",
        "menu.pause": "Pausar",
        "menu.resume": "Reanudar",
        "menu.reset": "Reiniciar",
        "menu.close_rom": "Cerrar juego",
        "menu.save_state": "Guardar estado",
        "menu.load_state": "Cargar estado",
        "menu.options": "&Opciones",
        "menu.options_dlg": "Opciones...",
        "menu.help": "A&yuda",
        "menu.controls": "Controles...",
        "menu.about": "Acerca de",
        "options.title": "Opciones",
        "options.audio": "Audio",
        "options.audio_enable": "Activar audio",
        "options.volume": "Volumen:",
        "options.controls": "Controles",
        "options.controls_desc": "Configuración de teclado y gamepad para el emulador.",
        "options.configure": "Configurar controles...",
        "options.general": "General",
        "options.library": "Biblioteca",
        "options.rom_folder": "Elegir carpeta de ROMs...",
        "options.language": "Idioma",
        "options.language_desc": "Idioma de la interfaz",
        "metadata.developer": "Desarrollador",
        "metadata.year": "Año",
        "metadata.genre": "Género",
        "metadata.players": "Jugadores",
        "metadata.region": "Región",
        "metadata.source": "Fuente",
        "metadata.api": "API",
        "metadata.local": "Local",
    },
    "en": {
        "splash.start": "Start",
        "splash.options": "Options",
        "splash.quit": "Quit Bitemu",
        "splash.credit": "Created by: Iván Ezequiel Rodriguez",
        "lobby.title": "Choose an edition",
        "lobby.back": "← Back",
        "lobby.soon": "Coming soon",
        "library.header": "Game Selection",
        "library.back": "← Back",
        "library.open_rom": "Open ROM...",
        "library.rom_folder": "ROM folder...",
        "library.search": "Search...",
        "library.empty": "No ROMs. Choose a ROM folder to get started.",
        "library.select_game": "Select a game",
        "library.load_rom": "Load ROM",
        "footer.back": "Back",
        "footer.load": "Load ROM",
        "menu.file": "&File",
        "menu.open_rom": "Open ROM...",
        "menu.recent": "Recent ROMs",
        "menu.none": "(none)",
        "menu.quit": "Quit",
        "menu.emulation": "&Emulation",
        "menu.pause": "Pause",
        "menu.resume": "Resume",
        "menu.reset": "Reset",
        "menu.close_rom": "Close game",
        "menu.save_state": "Save state",
        "menu.load_state": "Load state",
        "menu.options": "&Options",
        "menu.options_dlg": "Options...",
        "menu.help": "&Help",
        "menu.controls": "Controls...",
        "menu.about": "About",
        "options.title": "Options",
        "options.audio": "Audio",
        "options.audio_enable": "Enable audio",
        "options.volume": "Volume:",
        "options.controls": "Controls",
        "options.controls_desc": "Keyboard and gamepad configuration.",
        "options.configure": "Configure controls...",
        "options.general": "General",
        "options.library": "Library",
        "options.rom_folder": "Choose ROM folder...",
        "options.language": "Language",
        "options.language_desc": "Interface language",
        "metadata.developer": "Developer",
        "metadata.year": "Year",
        "metadata.genre": "Genre",
        "metadata.players": "Players",
        "metadata.region": "Region",
        "metadata.source": "Source",
        "metadata.api": "API",
        "metadata.local": "Local",
    },
}


def _current_lang() -> str:
    s = QSettings()
    lang = s.value(LANG_KEY, DEFAULT_LANG, type=str)
    return lang if lang in SUPPORTED else DEFAULT_LANG


def t(key: str) -> str:
    """Traduce una clave. Ej: t('splash.start') → 'Start' o 'Iniciar'."""
    lang = _current_lang()
    return _TRANSLATIONS.get(lang, _TRANSLATIONS["en"]).get(key, key)


def set_language(lang: str):
    if lang in SUPPORTED:
        QSettings().setValue(LANG_KEY, lang)


def current_language() -> str:
    return _current_lang()


def language_name(code: str) -> str:
    return {"es": "Español", "en": "English"}.get(code, code)
