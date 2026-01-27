# See the build system documentation in IDF programming guide
# for more information about component CMakeLists.txt files.

idf_component_register(
    SRCS main.c         # list the source files of this component
    INCLUDE_DIRS     "."   # optional, add here public include directories
    PRIV_INCLUDE_DIRS   # optional, add here private include directories
    REQUIRES          ds18x20 ds1307 esp_adc onewire sht3x # optional, list the public requirements (component names)
    PRIV_REQUIRES       # optional, list the private requirements
)
