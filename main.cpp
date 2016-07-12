
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/optional.hpp>

#include "Commandline.hxx"
#include "exceptions.hxx"
#include "Timer.hxx"
#include "PDF.hxx"

int main(int argc, const char **argv) {
    try {
        namespace ptree = boost::property_tree;

        Magick::InitializeMagick(NULL);

        boost::optional<Command> command_opt;
        try {
            // Parse all command line arguments
            command_opt = parse_options(argc, argv);
        } catch (std::exception &e) {
            std::cerr << e.what() << std::endl;
            return 1;
        }

        // Exit with 0 if command_opt returned boost::none - indicates success, but no valid settings
        if (!command_opt) {
            return 0;
        }

        auto command = *command_opt;

        // Read the
        ptree::ptree pt;
        try {
            ptree::read_json(boost::apply_visitor(generic_visitor{}, command), pt);
        } catch (std::exception &e) {
            throw invalid_config_exception(std::string("\tthe config file is malformed\n\tException: ") + e.what());
        }

        // try to fill our Options object with all provided options
        std::shared_ptr<Options> opts;
        try {
            opts = std::make_shared<Options>(
                pt.get<std::string>("inputPDF"),
                pt.get<int>("dpi"),
                pt.get_child_optional("parallel_processing") ? pt.get<bool>("parallel_processing") : false,
                pt.get<std::string>("language")
            );
            for (const auto &crop : pt.get_child("crops")) {
                opts->addCrop(Options::Crop{
                    crop.second.get<std::string>("type"),
                    crop.second.get<float>("dimensions.x"),
                    crop.second.get<float>("dimensions.y"),
                    crop.second.get<float>("dimensions.w"),
                    crop.second.get<float>("dimensions.h")
                });
            }
        } catch (std::exception &e) {
            throw invalid_config_exception(
                std::string("\tthe json format does not meet the expectations\n\tException: ") + e.what());
        }

        // check if the supplied crop types are unique
        {
            // Sorts all types in the vector and moves the duplicate ones to the back
            // Alters the underlying vector, but we don't care -> exit if duplicates are found
            auto it = std::unique(
                opts->_crops.begin(), opts->_crops.end(),
                [](const Options::Crop &a, const Options::Crop &b) {
                    return a.type == b.type;
                }
            );

            if (it != opts->_crops.end()) {
                std::cerr << "ERROR: duplicate crop types:" << std::endl;
                std::for_each(it, opts->_crops.end(), [](const Options::Crop &crop) {
                    std::cerr << "\t" << crop.type << std::endl;
                });
                return 1;
            }
        }

        // Subcommand handling below
        if (command.type() == typeid(PageCommand)) { // first_page subcommand handling
            auto c = boost::get<PageCommand>(command);
            PDF main_pdf(opts);

            if(c._page && c._page.get() >= main_pdf.page_count())
            {
                throw std::runtime_error("page cannot be bigger than the document's page count");
            }

            auto page = main_pdf.get_page(c._page ? c._page.get() : 0);
            auto img = page.get_image_representation();
            img.write(c._path);
        } else {

            if (command.type() == typeid(HumanCommand)) {
                auto c = boost::get<HumanCommand>(command);
                opts->_start = c._start;
                opts->_end = c._end;
                opts->_page = c._page;
            }
            if (command.type() == typeid(OutputCommand)) {
                auto c = boost::get<OutputCommand>(command);
                opts->_start = c._start;
                opts->_end = c._end;
                opts->_page = c._page;
            }

            PDF main_pdf(opts);
            // everything happens here
            auto result = main_pdf.work();

            if (command.type() == typeid(HumanCommand)) { // human subcommand handling

                for (auto &p : result.get_child("results")) {
                    std::cout << "Page " << p.second.get<std::string>("page") << ":" << std::endl;
                    for (auto &r : p.second.get_child("results")) {
                        std::cout << "Type: " << std::endl << r.second.get<std::string>("type") << std::endl
                                  << std::endl;
                        std::cout << "Result: " << std::endl << r.second.get<std::string>("value") << std::endl;
                    }
                }

            } else if (command.type() == typeid(OutputCommand)) { // output subcommand handling

                auto subcomm = boost::get<OutputCommand>(command);
                try {
                    std::ofstream ofs(subcomm._path, std::ios_base::trunc);
                    write_json(ofs, result);
                } catch (std::exception &e) {
                    throw std::runtime_error("could not write to " + subcomm._path + ": " + e.what());
                }

            } else {
                throw std::runtime_error("Invalid subcommand type. This should never happen, please file a bug report");
            }
        }
    } catch (std::exception &e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}

