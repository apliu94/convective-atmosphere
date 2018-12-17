#include <iomanip>
#include <sstream>
#include "json.hpp"
#include "app_utils.hpp"
#include "jic.hpp"
using namespace jic;




// ============================================================================
VISITABLE_STRUCT(run_status, time, wall, iter, vtk_count, chkpt_count);
VISITABLE_STRUCT(run_config,
    outdir,
    restart,
    tfinal,
    cpi,
    vtki,
    rk,
    nr,
    num_levels,
    threaded,
    test_mode,
    jet_opening_angle,
    jet_velocity,
    jet_density,
    density_index,
    temperature,
    outer_radius);




// ============================================================================
run_status run_status::from_file(std::string filename)
{
    if (! filesystem::isfile(filename))
    {
        return run_status();
    }
    auto ifs = std::ifstream(filename);
    return from_json(ifs);
}

run_status run_status::from_json(std::istream& is)
{
    auto j = nlohmann::json();
    auto status = run_status();

    is >> j;

    visit_struct::for_each(status, [j] (const char* name, auto& value)
    {
        value = j[name];
    });
    return status;
}

void run_status::print(std::ostream& os) const
{
    formatted_output::print_dotted(os, "Status", *this);
}

void run_status::tojson(std::ostream& os) const
{
    auto j = nlohmann::json();

    visit_struct::for_each(*this, [&j] (const char* name, auto& value)
    {
        j[name] = value;
    });
    os << std::setw(4) << j << std::endl;
}




// ============================================================================
void run_config::print(std::ostream& os) const
{
    formatted_output::print_dotted(os, "Config", *this);
}

void run_config::tojson(std::ostream& os) const
{
    auto j = nlohmann::json();

    visit_struct::for_each(*this, [&j] (const char* name, auto& value)
    {
        j[name] = value;
    });
    os << std::setw(4) << j << std::endl;
}

run_config run_config::validate() const
{
    if (nr < 4)             throw std::runtime_error("nr must be >= 4");
    if (rk != 1 && rk != 2) throw std::runtime_error("rk must be 1 or 2");
    if (outer_radius < 2.0) throw std::runtime_error("outer_radius must be > 2");
    return *this;
}

run_config run_config::from_json(std::istream& is)
{
    auto j = nlohmann::json();
    auto cfg = run_config();

    is >> j;

    visit_struct::for_each(cfg, [j] (const char* name, auto& value)
    {
        value = j[name];
    });
    return cfg;
}

run_config run_config::from_dict(std::map<std::string, std::string> items)
{
    auto cfg = run_config();

    visit_struct::for_each(cfg, [items] (const char* name, auto& value)
    {
        if (items.find(name) != items.end())
        {
            cmdline::set_from_string(items.at(name), value);
        }
    });

    for (const auto& item : items)
    {
        bool found = false;

        visit_struct::for_each(cfg, [item, &found] (const char* name, auto&)
        {
            if (item.first == name)
            {
                found = true;;
            }
        });

        if (! found)
        {
            throw std::runtime_error("unrecognized option: " + item.first);
        }
    }
    return cfg;
}

run_config run_config::from_argv(int argc, const char* argv[])
{
    auto args = cmdline::parse_keyval(argc, argv);
    auto cfg = run_config();

    if (args.count("restart"))
    {
        auto status_file = filesystem::join({args.at("restart"), "config.json"});
        auto ifs = std::ifstream(status_file);

        if (! ifs.is_open())
        {
            throw std::runtime_error("restart file not found: " + status_file);
        }
        cfg = from_json(ifs);
    }

    visit_struct::for_each(cfg, [items=args] (const char* name, auto& value)
    {
        if (items.find(name) != items.end())
        {
            cmdline::set_from_string(items.at(name), value);
        }
    });
    return cfg;
}

std::string run_config::make_filename_chkpt(int count) const
{
    if (count == -1)
    {
        return restart;
    }
    auto ss = std::stringstream();
    ss << "chkpt." << std::setfill('0') << std::setw(4) << count;
    return filesystem::join({outdir, ss.str()});
}

std::string run_config::make_filename_vtk(int count) const
{
    auto ss = std::stringstream();
    ss << std::setfill('0') << std::setw(4) << count << ".vtk";
    return filesystem::join({outdir, ss.str()});
}

std::string run_config::make_filename_status(int count) const
{
    return filesystem::join({make_filename_chkpt(count), "status.json"});
}

std::string run_config::make_filename_config(int count) const
{
    return filesystem::join({make_filename_chkpt(count), "config.json"});
}
