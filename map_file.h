#ifndef _MAP_FILE_H
#define _MAP_FILE_H

#ifdef __cplusplus
extern "C" {
#endif

void map_input_file(const char *filename, void **start, long *len);
void *map_temp_writable(size_t size);

#define map_in(ptr, len, path) map_input_file(path, (void**)&ptr, &len), len /= sizeof(*ptr)

#ifdef __cplusplus
}
#endif

#endif
