#ifndef OBJLOADERH
#define OBJLOADERH

u32 objloader_load(const char* file_path,
                   Array &out_vertex_array,
                   Array &out_uv_array,
                   Array &out_normal_array)
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

    Array temp_uv_array;
    array_init(temp_uv_array, 3 * sizeof(float), 1024*1024);

    Array temp_normal_array;
    array_init(temp_normal_array, 3 * sizeof(float), 1024*1024);

    while(fgets(line, 256, fp) != NULL) 
    {
        totalRead = strlen(line);
        /*
         * Trim new line character from last if exists.
         */
        line[totalRead - 1] = line[totalRead - 1] == '\n' ? '\0' : line[totalRead - 1];

        char data_type = line[0];
        char data_type_2 = line[1];
        if (data_type == 'v' && data_type_2 == ' ')
        {
            float vert[3] = {0};
            int matches = sscanf(line, "v %f %f %f\n", &vert[0], &vert[1], &vert[2]);
            printf("matches%i\n", matches);
            printf("adding %f %f %f\n", vert[0], vert[1], vert[2]);
            array_append(temp_vertex_array, &vert);
        }
        else if (data_type == 'v' && data_type_2 == 't')
        {
            float uv[3] = {0};
            int matches = sscanf(line, "vt %f %f %f\n", &uv[0], &uv[1], &uv[2]);
            array_append(temp_uv_array, &uv);
        }
        else if (data_type == 'v' && data_type_2 == 'n')
        {
            float normal[3] = {0};
            int matches = sscanf(line, "vn %f %f %f\n", &normal[0], &normal[1], &normal[2]);
            array_append(temp_normal_array, &normal);
        }
        else if (data_type == 'f')
        {
            u32 vert_idx[3] = {0};
            u32 uv_idx[3] = {0};
            u32 normal_idx[3] = {0};
            int matches = sscanf(line, "f %d %d %d\n", &vert_idx[0], &vert_idx[1], &vert_idx[2]);
            if(matches == 3)
            {
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
            else
            {
                u32 matches = sscanf(line, "f %d/%d/%d %d/%d/%d %d/%d/%d\n",
                    &vert_idx[0], &uv_idx[0], &normal_idx[0],
                    &vert_idx[1], &uv_idx[1], &normal_idx[1],
                    &vert_idx[2], &uv_idx[2], &normal_idx[2]);
                if(matches == 9)
                {
                    float* vert1 = (float*)array_get_index(temp_vertex_array, vert_idx[0] - 1);
                    float* vert2 = (float*)array_get_index(temp_vertex_array, vert_idx[1] - 1);
                    float* vert3 = (float*)array_get_index(temp_vertex_array, vert_idx[2] - 1);
                    array_extend(out_vertex_array, vert1, 3);
                    array_extend(out_vertex_array, vert2, 3);
                    array_extend(out_vertex_array, vert3, 3);

                    float* uv1 = (float*)array_get_index(temp_uv_array, uv_idx[0] - 1);
                    float* uv2 = (float*)array_get_index(temp_uv_array, uv_idx[1] - 1);
                    float* uv3 = (float*)array_get_index(temp_uv_array, uv_idx[2] - 1);
                    array_extend(out_uv_array, uv1, 3);
                    array_extend(out_uv_array, uv2, 3);
                    array_extend(out_uv_array, uv3, 3);

                    float* normal1 = (float*)array_get_index(temp_normal_array, normal_idx[0] - 1);
                    float* normal2 = (float*)array_get_index(temp_normal_array, normal_idx[1] - 1);
                    float* normal3 = (float*)array_get_index(temp_normal_array, normal_idx[2] - 1);
                    array_extend(out_normal_array, normal1, 3);
                    array_extend(out_normal_array, normal2, 3);
                    array_extend(out_normal_array, normal3, 3);
                }
                else
                {
                     return -1;
                }

            }
        }
    }
    array_free(temp_vertex_array);
    fclose(fp);

    return 0;
}
#endif //OBJLOADERH
