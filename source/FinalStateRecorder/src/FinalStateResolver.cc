#include "FinalStateResolver.h"

FinalStateResolver::FinalStateResolver(
    int process,
    std::vector<int> allowed_values,
    ResolverFunc resolver
){
    m_process = process;
    m_allowed_values = allowed_values;
    m_resolver = resolver;
}
