#include "pch.h"

#include "Asset.h"
#include "Utils.h"

std::string data_path = "C:\\Users\\natha\\OneDrive\\Desktop\\C++\\AgisCoreTest\\data";
TEST(AssetConstruct, SimpleConstruct) {
	auto asset = Asset("asset1", "exchange1");
	auto file_path = join_paths(data_path, "test1.csv");
	auto res = asset.load(file_path, "%Y-%m-%d");
	EXPECT_EQ(res, NexusStatusCode::Ok);
	EXPECT_EQ(asset.__get_close_index(), 1);
	EXPECT_EQ(asset.__get_open_index(), 0);

	auto headers = asset.get_headers();
	auto open_idx = headers["OPEN"];
	auto close_idx = headers["CLOSE"];
	EXPECT_EQ(open_idx,0);
	EXPECT_EQ(close_idx,1);
	EXPECT_EQ(asset.__get("OPEN", 0), 100);
	EXPECT_EQ(asset.__get("OPEN",1), 102);
	EXPECT_EQ(asset.__get("CLOSE", 0), 101);
	EXPECT_EQ(asset.__get("CLOSE", 1), 103);
	EXPECT_EQ(asset.__get_dt(0), 960267600000000000);
	EXPECT_EQ(asset.__get_dt(1), 960354000000000000);
}