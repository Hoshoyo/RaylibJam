static Rectangle grass_recs[13] = {
  {225.000000, 279.000000, 60.000000, 60.000000},
  {296.000000, 279.000000, 60.000000, 60.000000},
  {366.000000, 279.000000, 60.000000, 60.000000},
  {437.000000, 279.000000, 60.000000, 60.000000},
  {437.000000, 330.000000, 60.000000, 60.000000},
  {369.000000, 332.000000, 60.000000, 60.000000},
  {298.000000, 332.000000, 60.000000, 60.000000},
  {227.000000, 328.000000, 60.000000, 60.000000},
  {173.000000, 377.000000, 80.000000, 60.000000},
  {256.000000, 380.000000, 80.000000, 60.000000},
  {344.000000, 380.000000, 80.000000, 60.000000},
  {433.000000, 380.000000, 80.000000, 60.000000},
  {530.000000, 380.000000, 80.000000, 60.000000},
};
static AssetAtlas grass = { {"res/grass/GrassTuft1.png", false, {0}, {0}, {1, 1}, 1.0f, 1, {0}}, sizeof(grass_recs) / sizeof(*grass_recs), grass_recs};

static AssetAnimatedSprite character[] = {
  {
    { "./res/character/idle_front_char.png",        {{0}, 31, 6, 6, 180, 180, 1.000000f }, {0}, {-4.0f, 0.0f}, 0, 1.0f, 1 },
    { "./res/character/idle_front_char_shadow.png", {{0}, 31, 6, 6, 180, 180, 1.000000f }, {0}, {-4.0f, 0.0f}, 0, 1.0f, 1 },
  },
};

static Rectangle building_recs[6] = {
  {623.000000 / 2.0f, 53.000000 / 2.0f, 627.000000 / 2.0f, 627.000000 / 2.0f},
  {38.000000 / 2.0f, 506.000000 / 2.0f, 776.000000 / 2.0f, 760.000000 / 2.0f},
  {1250.000000 / 2.0f, 564.000000 / 2.0f, 776.000000 / 2.0f, 801.000000 / 2.0f},
  {1456.000000 / 2.0f, 1377.000000 / 2.0f, 586.000000 / 2.0f, 622.000000 / 2.0f},
  {803.000000 / 2.0f, 1284.000000 / 2.0f, 652.000000 / 2.0f, 713.000000 / 2.0f},
  {34.000000 / 2.0f, 1299.000000 / 2.0f, 742.000000 / 2.0f, 713.000000 / 2.0f},
};
static AssetAtlas buildings_albedo = { {"res/building/BuildingsIso1024x1024_Albedo.png", false, {0}, {0}, {1.0f, 1.0f}, 1.0f, 1, {0}}, sizeof(building_recs) / sizeof(*building_recs), building_recs};
static AssetAtlas buildings_shadow = { {"res/building/BuildingsIso1024x1024_Shadow.png", false, {0}, {0}, {1.0f, 1.0f}, 0.7f, 1, {0}}, sizeof(building_recs) / sizeof(*building_recs), building_recs};

/*
  const char* path;
  bool        loaded;
  Texture2D   texture;
  Vector2     origin;
  Vector2     scale;
  float       opacity;
  SpriteFlag  flags;
  Rectangle   src_rec;
*/