function(concat_files OUTPUT_FILE)
    set(options)
    set(oneValueArgs)
    set(multiValueArgs INPUT_FILES)
    cmake_parse_arguments(ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    # Ensure output directory exists
    get_filename_component(OUTPUT_DIR "${OUTPUT_FILE}" DIRECTORY)
    file(MAKE_DIRECTORY "${OUTPUT_DIR}")

    # Wipe the output file
    file(WRITE "${OUTPUT_FILE}" "")

    # Append each input file
    foreach(FILE IN LISTS ARG_INPUT_FILES)
        file(READ "${FILE}" CONTENT)
        file(APPEND "${OUTPUT_FILE}" "${CONTENT}")
    endforeach()
endfunction()
