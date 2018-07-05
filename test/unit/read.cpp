#include "gtest/gtest.h"

#include "config.hpp"
#include "verify.hpp"

#include <entwine/builder/builder.hpp>
#include <entwine/reader/reader.hpp>

namespace
{
    const Verify v;
}

TEST(read, count)
{
    const std::string out(test::dataPath() + "out/ellipsoid/ellipsoid-multi");

    {
        Config c;
        c["input"] = test::dataPath() + "ellipsoid.laz";
        c["output"] = out;
        c["force"] = true;
        c["hierarchyStep"] = static_cast<Json::UInt64>(v.hierarchyStep());
        c["ticks"] = static_cast<Json::UInt64>(v.ticks());

        Builder b(c);
        b.go();
    }

    Reader r(out);
    const Metadata& m(r.metadata());
    EXPECT_EQ(m.ticks(), v.ticks());
    EXPECT_EQ(m.hierarchyStep(), v.hierarchyStep());

    uint64_t np(0);
    for (std::size_t i(0); i < 8; ++i)
    {
        Json::Value q;
        q["bounds"] = m.boundsNativeCubic().get(toDir(i)).toJson();

        auto countQuery = r.count(q);
        countQuery->run();
        np += countQuery->numPoints();
    }

    EXPECT_EQ(np, v.numPoints());
}

TEST(read, data)
{
    const std::string out(test::dataPath() + "out/ellipsoid/ellipsoid-multi");

    {
        Config c;
        c["input"] = test::dataPath() + "ellipsoid.laz";
        c["output"] = out;
        c["force"] = true;
        c["hierarchyStep"] = static_cast<Json::UInt64>(v.hierarchyStep());
        c["ticks"] = static_cast<Json::UInt64>(v.ticks());

        Builder b(c);
        b.go();
    }

    Reader r(out);
    const Metadata& m(r.metadata());
    EXPECT_EQ(m.ticks(), v.ticks());
    EXPECT_EQ(m.hierarchyStep(), v.hierarchyStep());

    const Schema schema(DimList {
        pdal::Dimension::Id::X,
        pdal::Dimension::Id::Y,
        pdal::Dimension::Id::Z
    });

    auto append([&r](std::vector<char>& v, Json::Value j)
    {
        auto q(r.read(j));
        q->run();
        v.insert(v.end(), q->data().begin(), q->data().end());
    });

    Json::Value j;
    j["schema"] = schema.toJson();

    std::vector<char> bin;
    for (std::size_t i(0); i < 8; ++i)
    {
        j["bounds"] = m.boundsNativeCubic().get(toDir(i)).toJson();
        append(bin, j);
    }

    auto cmp([](const Point& a, const Point& b) { return ltChained(a, b); });
    using Counts = std::map<Point, uint64_t, decltype(cmp)>;

    auto count([&schema, &cmp](const std::vector<char>& v) -> Counts
    {
        Counts c(cmp);
        const std::size_t size(sizeof(double));
        for (std::size_t i(0); i < v.size(); i += schema.pointSize())
        {
            Point p;

            const char* pos(v.data() + i);
            std::copy(pos, pos + size, reinterpret_cast<char*>(&p.x));
            pos += size;
            std::copy(pos, pos + size, reinterpret_cast<char*>(&p.y));
            pos += size;
            std::copy(pos, pos + size, reinterpret_cast<char*>(&p.z));

            if (!c.count(p)) c[p] = 1;
            else ++c[p];
        }

        return c;
    });

    const Counts counts(count(bin));

    ASSERT_EQ(counts.size(), v.numPoints());
}

TEST(read, filter)
{
}

