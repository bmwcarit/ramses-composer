get_filename_component(EXECUTABLE ${TARGET_FILE} NAME)
configure_file("${ROOT_DIR}/ubuntustartscript.sh.in" "${TARGET_FILE}.sh" @ONLY)