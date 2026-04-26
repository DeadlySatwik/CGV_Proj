# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "/home/kyan/Downloads/cgv/CGV_Proj/without_traffic_light/build/_deps/glm_ext-src"
  "/home/kyan/Downloads/cgv/CGV_Proj/without_traffic_light/build/_deps/glm_ext-build"
  "/home/kyan/Downloads/cgv/CGV_Proj/without_traffic_light/build/_deps/glm_ext-subbuild/glm_ext-populate-prefix"
  "/home/kyan/Downloads/cgv/CGV_Proj/without_traffic_light/build/_deps/glm_ext-subbuild/glm_ext-populate-prefix/tmp"
  "/home/kyan/Downloads/cgv/CGV_Proj/without_traffic_light/build/_deps/glm_ext-subbuild/glm_ext-populate-prefix/src/glm_ext-populate-stamp"
  "/home/kyan/Downloads/cgv/CGV_Proj/without_traffic_light/build/_deps/glm_ext-subbuild/glm_ext-populate-prefix/src"
  "/home/kyan/Downloads/cgv/CGV_Proj/without_traffic_light/build/_deps/glm_ext-subbuild/glm_ext-populate-prefix/src/glm_ext-populate-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "/home/kyan/Downloads/cgv/CGV_Proj/without_traffic_light/build/_deps/glm_ext-subbuild/glm_ext-populate-prefix/src/glm_ext-populate-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "/home/kyan/Downloads/cgv/CGV_Proj/without_traffic_light/build/_deps/glm_ext-subbuild/glm_ext-populate-prefix/src/glm_ext-populate-stamp${cfgdir}") # cfgdir has leading slash
endif()
