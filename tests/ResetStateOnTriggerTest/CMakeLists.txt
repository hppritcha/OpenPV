set(TEST_PARAMS "ResetStateOnTriggerTest")

if (PV_USE_CUDA)
  set(TEST_PARAMS "${TEST_PARAMS} ResetStateOnTriggerTestGPU")
endif()

pv_add_test(PARAMS ${testParams})

