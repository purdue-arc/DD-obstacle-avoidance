// header file
#include "header.hh"

// functions
// Compare function determines which node is a better choice to expand upon
bool nodeCompare(const node_t* n1, const node_t* n2) {
    if ((n1->heuristic + n1->cost) > (n2->heuristic + n2->cost)) {
        return true;
    }
    return false;
} 

// checks if given stream is supported
bool device_with_streams(std::vector <rs2_stream> stream_requests, std::string& out_serial) {
    context current_context;
    auto devices = current_context.query_devices();
    vector <rs2_stream> unavailable_streams = stream_requests;
    for (auto dev : devices) {
        map<rs2_stream, bool> found_streams;
        for (auto& type : stream_requests) {
            found_streams[type] = false;
            for (auto& sensor : dev.query_sensors()) {
                for (auto& profile : sensor.get_stream_profiles()) {
                    if (profile.stream_type() == type) {
                        found_streams[type] = true;
                        unavailable_streams.erase(std::remove(unavailable_streams.begin(), unavailable_streams.end(), type), unavailable_streams.end());
                        if (dev.supports(RS2_CAMERA_INFO_SERIAL_NUMBER))
                            out_serial = dev.get_info(RS2_CAMERA_INFO_SERIAL_NUMBER);
                    }
                }
            }
        }

        for (auto& stream : found_streams) {
            if (!stream.second) {
                return false;
            }
        }
        return true;
    }
}

// converts a realsense frame to opencv viewable "mat"
Mat frame_to_mat(frame frame) {
    Mat color_image(Size(WIDTH, HEIGHT), CV_8UC3, (void*)frame.get_data(), Mat::AUTO_STEP);
    return color_image;
}