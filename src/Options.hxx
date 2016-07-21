//
// Created by markus on 30/06/16.
//

#ifndef CONVERTER_OPTIONS_HXX
#define CONVERTER_OPTIONS_HXX

#include <vector>
#include <string>

#include <boost/optional.hpp>

/**
 * Helper struct for all options included in the supplied config file.
 */
struct Options {
    /**
     * One set of crop settings:
     * - type: the type of the area, any string
     * - x: x-coordinate in percent
     * - y: y-coordinate in percent
     * - w: width in percent
     * - h: height in percent
     */
    struct Crop {
        Crop(const std::string &type_, float x_, float y_, float w_, float h_)
            : type(type_), x(x_), y(y_), w(w_), h(h_) {}

        std::string type;
        float x;
        float y;
        float w;
        float h;
    };

    Options(const std::string &inputPDF, int dpi, bool parallel_processing,
            std::string language)
        : _inputPDF(inputPDF), _dpi(dpi),
          _parallel_processing(parallel_processing),
          _language(language), _start(-1), _end(-1) {}

    void addCrop(Crop crop) {
        _crops.push_back(crop);
    }

    std::vector<Crop> _crops;
    std::string _inputPDF;
    int _dpi;
    bool _parallel_processing;
    std::string _language;
    boost::optional<int> _start;
    boost::optional<int> _end;
    boost::optional<int> _page;
};

#endif //CONVERTER_OPTIONS_HXX