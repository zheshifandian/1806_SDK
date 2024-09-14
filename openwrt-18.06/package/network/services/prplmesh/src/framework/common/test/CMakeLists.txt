if(BUILD_TESTS)

	# Encryption Tests
	set(TEST_NAME mapf_common_encryption_tests)
    add_executable(${TEST_NAME}
        encryption_test.cpp
    )

    target_link_libraries(${TEST_NAME} mapfcommon elpp)

    install(TARGETS ${TEST_NAME} DESTINATION tests)
    add_test(NAME ${TEST_NAME} COMMAND $<TARGET_FILE:${TEST_NAME}>)
endif()
