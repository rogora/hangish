TRANSLATIONS += \
        translations/harbour-hangish-it.ts \
        translations/harbour-hangish-en.ts \
        translations/harbour-hangish-sv.ts

OTHER_FILES += translations/translations.json

updateqm.input = TRANSLATIONS
updateqm.output = translations/${QMAKE_FILE_BASE}.qm
updateqm.commands = \
	lrelease -idbased ${QMAKE_FILE_IN} \
        -qm translations/${QMAKE_FILE_BASE}.qm
updateqm.CONFIG += no_link
QMAKE_EXTRA_COMPILERS += updateqm

PRE_TARGETDEPS += compiler_updateqm_make_all

localization.files = $$files(translations/*.qm)
localization.path = /usr/share/harbour-hangish/translations

INSTALLS += localization
