
set( FILE_LIST
    sip_parser.cc
    sip_parser.h
    sip_dialog.cc
    sip_dialog.h
    sip_utils.cc
    sip_utils.h
    sip.cc
    sip.h
    sip_roptions.h
    sip_common.h
    sip_config.cc
    sip_config.h
    sip_module.cc
    sip_module.h
    ips_sip.cc
    ips_sip_stat_code.cc
    ips_sip_method.cc
)

if (STATIC_INSPECTORS)
    add_library( sip STATIC ${FILE_LIST})

else (STATIC_INSPECTORS)
    add_shared_library(sip inspectors ${FILE_LIST})

endif (STATIC_INSPECTORS)
