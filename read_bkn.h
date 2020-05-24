#ifndef READ_BKN_H
#define READ_BKN_H

struct data_point {
    float time; 
    float absorbance;
};

struct data_set {
    struct data_point* points;
    int numPoints;
    char** metadata;
    int numMetadata;
};

#endif