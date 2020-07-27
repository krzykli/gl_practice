#ifndef OBJLOADERH
#define OBJLOADERH

u32 objloader_load(const char* file_path, Array &out_vertex_array)
{
    FILE *fp;
    char line[256];
    u32 totalRead = 0;

    /* opening file for reading */
    fp = fopen(file_path, "r");
    if(fp == NULL) {
        perror("Error opening file");
        return(-1);
    }

    Array temp_vertex_array;
    array_init(temp_vertex_array, 3 * sizeof(float), 1024*1024);

    while(fgets(line, 256, fp) != NULL) 
    {
        totalRead = strlen(line);
        /*
         * Trim new line character from last if exists.
         */
        line[totalRead - 1] = line[totalRead - 1] == '\n' ? '\0' : line[totalRead - 1];

        char data_type = line[0];
        if (data_type == 'v')
        {
            char* offset_line = &line[2];
            char* token = strtok(offset_line, " ");
            u8 i = 0;
            float vert[3] = {0};
            while( token != NULL ) {
                float result = atof(token);
                vert[i] = result;
                token = strtok(NULL, " ");
                i++;
            }
            //printf("adding %f %f %f\n", vert[0], vert[1], vert[2]);
            array_append(temp_vertex_array, &vert);
        }
        // else if (data_type == 'vt')  TODO
        // else if (data_type == 'vn')  TODO
        else if (data_type == 'f')
        {
            u32 vert_idx[3] = {0};
            char* offset_line = &line[2];
            char* token = strtok(offset_line, " ");
            u8 i = 0;
            while( token != NULL ) {
                u32 result = atoi(token);
                vert_idx[i] = result;
                token = strtok(NULL, " ");
                i++;
            }
            float* vert1 = (float*)array_get_index(temp_vertex_array, vert_idx[0] - 1);
            float* vert2 = (float*)array_get_index(temp_vertex_array, vert_idx[1] - 1);
            float* vert3 = (float*)array_get_index(temp_vertex_array, vert_idx[2] - 1);
            //printf("%d %d %d\n", vert_idx[0], vert_idx[1], vert_idx[2]);
            //printf("loading %f %f %f\n", vert1[0], vert1[1], vert1[2]);
            //printf("loading %f %f %f\n", vert2[0], vert2[1], vert2[2]);
            //printf("loading %f %f %f\n", vert3[0], vert3[1], vert3[2]);
            array_extend(out_vertex_array, vert1, 3);
            array_extend(out_vertex_array, vert2, 3);
            array_extend(out_vertex_array, vert3, 3);
        }
    }
    array_free(temp_vertex_array);
    fclose(fp);

    return 0;
}
#endif //OBJLOADERH
