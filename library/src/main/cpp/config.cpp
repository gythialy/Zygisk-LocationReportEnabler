#include <map>
#include <vector>
#include <string>
#include <fcntl.h>
#include <unistd.h>
#include <sys/system_properties.h>

#include "config.h"
#include "misc.h"
#include "logging.h"

using namespace Config;

namespace Config {

    namespace Properties {

        void Put(const char *name, const char *value);
    }

    namespace Packages {

        void Add(const char *name);
    }
	
}

static std::string packagename;
static std::map<std::string, Property *> props;
static std::vector<std::string> packages;

Property *Properties::Find(const char *name) {
    if (!name) return nullptr;

    auto it = props.find(name);
    if (it != props.end()) {
        return it->second;
    }
    return nullptr;
}

void Properties::Put(const char *name, const char *value) {
    if (!name) return;

    auto prop = Find(name);
    delete prop;

    props[name] = new Property(name, value ? value : "");

    LOGD("property: %s %s", name, value);
}

bool Packages::Find(const char *name) {
    if (!name) return false;
    return std::find(packages.begin(), packages.end(), name) != packages.end();
}

void Packages::Add(const char *name) {
    if (!name) return;
    packages.emplace_back(name);

    LOGD("package: %s", name);
}

void Config::Load(const int module_fd) {
	int config_fd;
	int properties_fd;
	int packages_fd;
	DIR *config_dir = opendirat(module_fd, "config", 0, &config_fd);
	DIR *properties_dir = opendirat(config_fd, "properties", 0, &properties_fd);
	DIR *packages_dir = opendirat(config_fd, "packages", 0, &packages_fd);

    //snprintf(buf, PATH_MAX, "%s/config/properties", module_path);
    DIRforeach_dir(properties_dir, [](int dirfd, struct dirent *entry, bool *) {
        auto name = entry->d_name;
        int fd = openat(dirfd, name, O_RDONLY);
        if (fd == -1) return;

        char buf[PROP_VALUE_MAX]{0};
        if (read(fd, buf, PROP_VALUE_MAX) >= 0) {
            Properties::Put(name, buf);
        }

        close(fd);
    });
	close(properties_fd);

    //snprintf(buf, PATH_MAX, "%s/config/packages", module_path);
    DIRforeach_dir(packages_dir, [](int, struct dirent *entry, bool *) {
        auto name = entry->d_name;
        Packages::Add(name);
    });
	close(packages_fd);
	
	close(config_fd);
}

void Config::SetPackageName(const char *name) {
	if (!name) return;
	packagename = name;
}

std::string Config::GetPackageName() {
	return packagename;
}