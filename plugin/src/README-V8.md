# Command line for building V8
python build\gyp_v8 -Dtarget_arch=ia32 -Dcomponent=shared_library -Dv8_use_snapshot=0 -Dv8_use_external_startup_data=0 -Dv8_enable_i18n_support=0