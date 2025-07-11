// clang-format off
//
// Created by goksu on 4/6/19.
//

#include <algorithm>
#include <vector>
#include "rasterizer.hpp"
#include <opencv2/opencv.hpp>
#include <math.h>


rst::pos_buf_id rst::rasterizer::load_positions(const std::vector<Eigen::Vector3f> &positions)
{
    auto id = get_next_id();
    pos_buf.emplace(id, positions);

    return {id};
}

rst::ind_buf_id rst::rasterizer::load_indices(const std::vector<Eigen::Vector3i> &indices)
{
    auto id = get_next_id();
    ind_buf.emplace(id, indices);

    return {id};
}

rst::col_buf_id rst::rasterizer::load_colors(const std::vector<Eigen::Vector3f> &cols)
{
    auto id = get_next_id();
    col_buf.emplace(id, cols);

    return {id};
}

auto to_vec4(const Eigen::Vector3f& v3, float w = 1.0f)
{
    return Vector4f(v3.x(), v3.y(), v3.z(), w);
}

static bool insideTriangle(float x, float y, const Vector3f* _v)
{
    Eigen::Vector3f p (x, y, 0.0f);

    Eigen::Vector3f v1 (_v[1].x(), _v[1].y(), 0);
    Eigen::Vector3f v0 (_v[0].x(), _v[0].y(), 0);
    Eigen::Vector3f v2 (_v[2].x(), _v[2].y(), 0);

    Eigen::Vector3f l1 = v0 - v1;
    Eigen::Vector3f l2 = v1 - v2;
    Eigen::Vector3f l3 = v2 - v0;

    int check1, check2, check3;

    check1 = (l1.cross(p - v0).z() >= 0) ? 1 : 0;
    check2 = (l2.cross(p - v1).z() >= 0) ? 1 : 0;
    check3 = (l3.cross(p - v2).z() >= 0) ? 1 : 0;

    if (check1 == check2 && check2 == check3)
        return true;

    return false;
}

// Preserved to be compatible with other hw
static bool insideTriangle(int pixel_x, int pixel_y, const Vector3f* _v)
{   
    // TODO : Implement this function to check if the point (x, y) is inside the triangle represented by _v[0], _v[1], _v[2]
    Eigen::Vector3f p ((float) pixel_x + 0.5, (float) pixel_y + 0.5, 0);

    Eigen::Vector3f v1 (_v[1].x(), _v[1].y(), 0);
    Eigen::Vector3f v0 (_v[0].x(), _v[0].y(), 0);
    Eigen::Vector3f v2 (_v[2].x(), _v[2].y(), 0);

    Eigen::Vector3f l1 = v0 - v1;
    Eigen::Vector3f l2 = v1 - v2;
    Eigen::Vector3f l3 = v2 - v0;

    int check1, check2, check3;

    check1 = (l1.cross(p - v0).z() >= 0) ? 1 : 0;
    check2 = (l2.cross(p - v1).z() >= 0) ? 1 : 0;
    check3 = (l3.cross(p - v2).z() >= 0) ? 1 : 0;

    if (check1 == check2 && check2 == check3)
        return true;
    
    return false;
}

static std::tuple<float, float, float> computeBarycentric2D(float x, float y, const Vector3f* v)
{
    float c1 = (x*(v[1].y() - v[2].y()) + (v[2].x() - v[1].x())*y + v[1].x()*v[2].y() - v[2].x()*v[1].y()) / (v[0].x()*(v[1].y() - v[2].y()) + (v[2].x() - v[1].x())*v[0].y() + v[1].x()*v[2].y() - v[2].x()*v[1].y());
    float c2 = (x*(v[2].y() - v[0].y()) + (v[0].x() - v[2].x())*y + v[2].x()*v[0].y() - v[0].x()*v[2].y()) / (v[1].x()*(v[2].y() - v[0].y()) + (v[0].x() - v[2].x())*v[1].y() + v[2].x()*v[0].y() - v[0].x()*v[2].y());
    float c3 = (x*(v[0].y() - v[1].y()) + (v[1].x() - v[0].x())*y + v[0].x()*v[1].y() - v[1].x()*v[0].y()) / (v[2].x()*(v[0].y() - v[1].y()) + (v[1].x() - v[0].x())*v[2].y() + v[0].x()*v[1].y() - v[1].x()*v[0].y());
    return {c1,c2,c3};
}

void rst::rasterizer::draw(pos_buf_id pos_buffer, ind_buf_id ind_buffer, col_buf_id col_buffer, Primitive type)
{
    auto& buf = pos_buf[pos_buffer.pos_id];
    auto& ind = ind_buf[ind_buffer.ind_id];
    auto& col = col_buf[col_buffer.col_id];

    float f1 = (50 - 0.1) / 2.0;
    float f2 = (50 + 0.1) / 2.0;

    Eigen::Matrix4f mvp = projection * view * model;
    for (auto& i : ind)
    {
        Triangle t;
        Eigen::Vector4f v[] = {
                mvp * to_vec4(buf[i[0]], 1.0f),
                mvp * to_vec4(buf[i[1]], 1.0f),
                mvp * to_vec4(buf[i[2]], 1.0f)
        };
        //Homogeneous division
        for (auto& vec : v) {
            vec /= vec.w();
        }
        //Viewport transformation
        for (auto & vert : v)
        {
            vert.x() = 0.5*width*(vert.x()+1.0);
            vert.y() = 0.5*height*(vert.y()+1.0);
            vert.z() = vert.z() * f1 + f2;
        }

        for (int i = 0; i < 3; ++i)
        {
            t.setVertex(i, v[i].head<3>());
            t.setVertex(i, v[i].head<3>());
            t.setVertex(i, v[i].head<3>());
        }

        auto col_x = col[i[0]];
        auto col_y = col[i[1]];
        auto col_z = col[i[2]];

        t.setColor(0, col_x[0], col_x[1], col_x[2]);
        t.setColor(1, col_y[0], col_y[1], col_y[2]);
        t.setColor(2, col_z[0], col_z[1], col_z[2]);

        rasterize_triangle(t);
    }
}

void rst::rasterizer::rasterize_triangle(const Triangle& t) 
{
    auto v = t.toVector4();

    Vector3f vertices[] = {t.v[0], t.v[1], t.v[2]};

    // 4 directions in super sampling
    auto directions = std::array{
        std::array{-0.25f, -0.25f},
        std::array{-0.25f,  0.25f},
        std::array{ 0.25f, -0.25f},
        std::array{ 0.25f,  0.25f}
    };

    // find bounding box
    int l = std::floor(std::min(std::min(t.v[0].x(), t.v[1].x()), t.v[2].x()));
    // compute the strict upper bound of the box
    int r = std::ceil(std::max(std::max(t.v[0].x(), t.v[1].x()), t.v[2].x()));
    int b = std::floor(std::min(std::min(t.v[0].y(), t.v[1].y()), t.v[2].y()));
    int top = std::ceil(std::max(std::max(t.v[0].y(), t.v[1].y()), t.v[2].y()));

    l = std::max(0, l);
    r = std::min(width, r);
    b = std::max(0, b);
    top = std::min(height, top);

    // iterate through pixel coordinates
    for (int x = l; x < r; x++) 
    {
        for (int y = b; y < top; y++) 
        {
            int index = get_index(x, y);

            // computer interpolated z
            auto[alpha, beta, gamma] = computeBarycentric2D((float) x + 0.5, (float) y + 0.5, t.v);
            // float w_reciprocal = 1.0/(alpha / v[0].w() + beta / v[1].w() + gamma / v[2].w());
            // float z_interpolated = alpha * v[0].z() / v[0].w() + beta * v[1].z() / v[1].w() + gamma * v[2].z() / v[2].w();
            // z_interpolated *= w_reciprocal;
        
            float z_interpolated = alpha * v[0].z() + beta * v[1].z() + gamma * v[2].z();
            z_interpolated *= -1;

            // occlusions
            if (z_interpolated > depth_buf[index])
            {
                continue;
            }

            int count = 0;

            // 4 samplings per pixel
            for (const auto& direction: directions) 
            {
                float sample_x = (float) x + 0.5f + direction[0];
                float sample_y = (float) y + 0.5f + direction[1];

                bool in_tri = insideTriangle(sample_x, sample_y, vertices);

                if (in_tri)
                {
                    count++;
                }
            }

            if (count > 0) 
            {
                float percentage = (float) count / 4;

                Eigen::Vector3f point ((float) x, (float) y, z_interpolated);

                set_pixel(point, t.getColor() * percentage);
                    
                depth_buf[index] = z_interpolated;
            }
        }
    }
}

void rst::rasterizer::set_model(const Eigen::Matrix4f& m)
{
    model = m;
}

void rst::rasterizer::set_view(const Eigen::Matrix4f& v)
{
    view = v;
}

void rst::rasterizer::set_projection(const Eigen::Matrix4f& p)
{
    projection = p;
}

void rst::rasterizer::clear(rst::Buffers buff)
{
    if ((buff & rst::Buffers::Color) == rst::Buffers::Color)
    {
        std::fill(frame_buf.begin(), frame_buf.end(), Eigen::Vector3f{0, 0, 0});
    }
    if ((buff & rst::Buffers::Depth) == rst::Buffers::Depth)
    {
        std::fill(depth_buf.begin(), depth_buf.end(), std::numeric_limits<float>::infinity());
    }
}

rst::rasterizer::rasterizer(int w, int h) : width(w), height(h)
{
    frame_buf.resize(w * h);
    depth_buf.resize(w * h);
}

int rst::rasterizer::get_index(int x, int y)
{
    return (height-1-y)*width + x;
}

void rst::rasterizer::set_pixel(const Eigen::Vector3f& point, const Eigen::Vector3f& color)
{
    //old index: auto ind = point.y() + point.x() * width;
    auto ind = (height-1-point.y())*width + point.x();
    frame_buf[ind] = color;

}

// clang-format on