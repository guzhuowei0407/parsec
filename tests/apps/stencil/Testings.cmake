parsec_addtest_cmd(apps/stencil ${SHM_TEST_CMD_LIST} apps/stencil/testing_stencil_1D -t 100 -T 100 -N 1000 -M 1000 -I 10 -R 2 -m 1)
if( MPI_C_FOUND )
  parsec_addtest_cmd(apps/stencil:mp ${MPI_TEST_CMD_LIST} 8 apps/stencil/testing_stencil_1D -t 100 -T 100 -N 1000 -M 1000 -I 10 -R 2 -m 1)
  if(TEST apps/stencil:mp)
    set_tests_properties(apps/stencil:mp PROPERTIES DEPENDS launch:mp)
  endif()
endif( MPI_C_FOUND )