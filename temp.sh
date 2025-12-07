function pingme() {
    local msg="${1:-Job Done!}"
    curl -d "$msg" ntfy.sh/index_gen
}

pingme "hi h"