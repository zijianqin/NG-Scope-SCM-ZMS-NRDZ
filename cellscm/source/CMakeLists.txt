#
# Copyright 2013-2021 Software Radio Systems Limited
#
# This file is part of srsRAN
#
# srsRAN is free software: you can redistribute it and/or modify
# it under the terms of the GNU Affero General Public License as
# published by the Free Software Foundation, either version 3 of
# the License, or (at your option) any later version.
#
# srsRAN is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU Affero General Public License for more details.
#
# A copy of the GNU Affero General Public License can be found in
# the LICENSE file in the top-level directory of this distribution
# and at http://www.gnu.org/licenses/.
#

if (RPATH)
  set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
endif (RPATH)

add_executable(cellscm "mib_decode.c" "main.cc" )

set(SRSRAN_SOURCES asn1_utils srsran_common srsran_mac srsran_phy srsran_radio srsran_gtpu  srsran_rlc srsran_pdcp rrc_asn1 srslog support system)

target_link_libraries(cellscm ${SRSRAN_SOURCES}
                              ${CMAKE_THREAD_LIBS_INIT}
                              ${LIBCONFIG_LIBRARY}
                              ${ATOMIC_LIBS})                       

if (RPATH)
  set_target_properties(cellscm PROPERTIES INSTALL_RPATH ".")
endif (RPATH)

install(TARGETS ngscope DESTINATION ${RUNTIME_DIR})

