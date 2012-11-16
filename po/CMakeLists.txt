# for i18n

SET(POFILES hy.po az.po eu.po bg.po ca.po zh_CN.po zh_HK.po zh_TW.po hr.po cs.po da.po nl_NL.po en.po en_US.po et.po fi.po fr_FR.po gl.po ka.po de_DE.po el_GR.po hu.po is.po ga.po it_IT.po ja_JP.po kk.po ko_KR.po lv.po lt.po mk.po nb.po pl.po pt_PT.po pt_BR.po ro.po ru_RU.po sr.po sk.po sl.po es_ES.po es_US.po sv.po  tr_TR.po uk.po uz.po ar.po zh_SG.po hi.po en_PH.po fr_CA.po)

SET(MSGFMT "/usr/bin/msgfmt")

FOREACH(pofile ${POFILES})
	SET(pofile ${CMAKE_CURRENT_SOURCE_DIR}/${pofile})
	MESSAGE("PO: ${pofile}")
	GET_FILENAME_COMPONENT(absPofile ${pofile} ABSOLUTE)
	GET_FILENAME_COMPONENT(lang ${absPofile} NAME_WE)
	SET(moFile ${CMAKE_CURRENT_BINARY_DIR}/${lang}.mo)
	ADD_CUSTOM_COMMAND(
			OUTPUT ${moFile}
			COMMAND ${MSGFMT} -o ${moFile} ${absPofile}
			DEPENDS ${absPofile}
	)
	INSTALL(FILES ${moFile}
			DESTINATION ${LOCALEDIR}/${lang}/LC_MESSAGES RENAME ${PROJECT_NAME}.mo)
	SET(moFiles ${moFiles} ${moFile})
ENDFOREACH(pofile)

MESSAGE(".mo files: ${moFiles}")
ADD_CUSTOM_TARGET(po ALL DEPENDS ${moFiles})
