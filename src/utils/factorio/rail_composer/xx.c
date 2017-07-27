

/* 
rail_pictures_internal = function(elems)
  local keys = {{"straight_rail", "horizontal", 64, 128, 0, 0, true},
                {"straight_rail", "vertical", 128, 64, 0, 0, true},
                {"straight_rail", "diagonal-left-top", 96, 96, 0.5, 0.5, true},
                {"straight_rail", "diagonal-right-top", 96, 96, -0.5, 0.5, true},
                {"straight_rail", "diagonal-right-bottom", 96, 96, -0.5, -0.5, true},
                {"straight_rail", "diagonal-left-bottom", 96, 96, 0.5, -0.5, true},
                {"curved_rail", "vertical-left-top", 192, 288, 0.5, 0.5},
                {"curved_rail", "vertical-right-top", 192, 288, -0.5, 0.5},
                {"curved_rail", "vertical-right-bottom", 192, 288, -0.5, -0.5},
                {"curved_rail", "vertical-left-bottom", 192, 288, 0.5, -0.5},
                {"curved_rail" ,"horizontal-left-top", 288, 192, 0.5, 0.5},
                {"curved_rail" ,"horizontal-right-top", 288, 192, -0.5, 0.5},
                {"curved_rail" ,"horizontal-right-bottom", 288, 192, -0.5, -0.5},
                {"curved_rail" ,"horizontal-left-bottom", 288, 192, 0.5, -0.5}}
*/

/* diag - left-bottom + right-top
          left-top + right-bottom

var base_s = "M:/tmp/xx/JunkTrain_0.0.8/graphics/straight-rail-wood/";
var base_c = "M:/tmp/xx/JunkTrain_0.0.8/graphics/curved-rail-wood/";

var rails = ["M:/tmp/xx/JunkTrain_0.0.8/graphics/curved-rail-wood/curved-rail-horizontal-left-bottom-backplates-remnants.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/curved-rail-wood/curved-rail-horizontal-left-bottom-backplates.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/curved-rail-wood/curved-rail-horizontal-left-bottom-metals-remnants.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/curved-rail-wood/curved-rail-horizontal-left-bottom-metals.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/curved-rail-wood/curved-rail-horizontal-left-bottom-stone-path-background-remnants.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/curved-rail-wood/curved-rail-horizontal-left-bottom-stone-path-background.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/curved-rail-wood/curved-rail-horizontal-left-bottom-stone-path-remnants.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/curved-rail-wood/curved-rail-horizontal-left-bottom-stone-path.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/curved-rail-wood/curved-rail-horizontal-left-bottom-ties-remnants.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/curved-rail-wood/curved-rail-horizontal-left-bottom-ties.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/curved-rail-wood/curved-rail-horizontal-left-top-backplates-remnants.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/curved-rail-wood/curved-rail-horizontal-left-top-backplates.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/curved-rail-wood/curved-rail-horizontal-left-top-metals-remnants.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/curved-rail-wood/curved-rail-horizontal-left-top-metals.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/curved-rail-wood/curved-rail-horizontal-left-top-stone-path-background-remnants.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/curved-rail-wood/curved-rail-horizontal-left-top-stone-path-background.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/curved-rail-wood/curved-rail-horizontal-left-top-stone-path-remnants.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/curved-rail-wood/curved-rail-horizontal-left-top-stone-path.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/curved-rail-wood/curved-rail-horizontal-left-top-ties-remnants.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/curved-rail-wood/curved-rail-horizontal-left-top-ties.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/curved-rail-wood/curved-rail-horizontal-right-bottom-backplates-remnants.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/curved-rail-wood/curved-rail-horizontal-right-bottom-backplates.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/curved-rail-wood/curved-rail-horizontal-right-bottom-metals-remnants.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/curved-rail-wood/curved-rail-horizontal-right-bottom-metals.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/curved-rail-wood/curved-rail-horizontal-right-bottom-stone-path-background-remnants.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/curved-rail-wood/curved-rail-horizontal-right-bottom-stone-path-background.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/curved-rail-wood/curved-rail-horizontal-right-bottom-stone-path-remnants.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/curved-rail-wood/curved-rail-horizontal-right-bottom-stone-path.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/curved-rail-wood/curved-rail-horizontal-right-bottom-ties-remnants.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/curved-rail-wood/curved-rail-horizontal-right-bottom-ties.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/curved-rail-wood/curved-rail-horizontal-right-top-backplates-remnants.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/curved-rail-wood/curved-rail-horizontal-right-top-backplates.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/curved-rail-wood/curved-rail-horizontal-right-top-metals-remnants.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/curved-rail-wood/curved-rail-horizontal-right-top-metals.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/curved-rail-wood/curved-rail-horizontal-right-top-stone-path-background-remnants.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/curved-rail-wood/curved-rail-horizontal-right-top-stone-path-background.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/curved-rail-wood/curved-rail-horizontal-right-top-stone-path-remnants.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/curved-rail-wood/curved-rail-horizontal-right-top-stone-path.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/curved-rail-wood/curved-rail-horizontal-right-top-ties-remnants.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/curved-rail-wood/curved-rail-horizontal-right-top-ties.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/curved-rail-wood/curved-rail-vertical-left-bottom-backplates-remnants.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/curved-rail-wood/curved-rail-vertical-left-bottom-backplates.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/curved-rail-wood/curved-rail-vertical-left-bottom-metals-remnants.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/curved-rail-wood/curved-rail-vertical-left-bottom-metals.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/curved-rail-wood/curved-rail-vertical-left-bottom-stone-path-background-remnants.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/curved-rail-wood/curved-rail-vertical-left-bottom-stone-path-background.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/curved-rail-wood/curved-rail-vertical-left-bottom-stone-path-remnants.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/curved-rail-wood/curved-rail-vertical-left-bottom-stone-path.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/curved-rail-wood/curved-rail-vertical-left-bottom-ties-remnants.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/curved-rail-wood/curved-rail-vertical-left-bottom-ties.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/curved-rail-wood/curved-rail-vertical-left-top-backplates-remnants.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/curved-rail-wood/curved-rail-vertical-left-top-backplates.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/curved-rail-wood/curved-rail-vertical-left-top-metals-remnants.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/curved-rail-wood/curved-rail-vertical-left-top-metals.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/curved-rail-wood/curved-rail-vertical-left-top-stone-path-background-remnants.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/curved-rail-wood/curved-rail-vertical-left-top-stone-path-background.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/curved-rail-wood/curved-rail-vertical-left-top-stone-path-remnants.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/curved-rail-wood/curved-rail-vertical-left-top-stone-path.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/curved-rail-wood/curved-rail-vertical-left-top-ties-remnants.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/curved-rail-wood/curved-rail-vertical-left-top-ties.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/curved-rail-wood/curved-rail-vertical-right-bottom-backplates-remnants.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/curved-rail-wood/curved-rail-vertical-right-bottom-backplates.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/curved-rail-wood/curved-rail-vertical-right-bottom-metals-remnants.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/curved-rail-wood/curved-rail-vertical-right-bottom-metals.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/curved-rail-wood/curved-rail-vertical-right-bottom-stone-path-background-remnants.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/curved-rail-wood/curved-rail-vertical-right-bottom-stone-path-background.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/curved-rail-wood/curved-rail-vertical-right-bottom-stone-path-remnants.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/curved-rail-wood/curved-rail-vertical-right-bottom-stone-path.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/curved-rail-wood/curved-rail-vertical-right-bottom-ties-remnants.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/curved-rail-wood/curved-rail-vertical-right-bottom-ties.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/curved-rail-wood/curved-rail-vertical-right-top-backplates-remnants.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/curved-rail-wood/curved-rail-vertical-right-top-backplates.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/curved-rail-wood/curved-rail-vertical-right-top-metals-remnants.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/curved-rail-wood/curved-rail-vertical-right-top-metals.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/curved-rail-wood/curved-rail-vertical-right-top-stone-path-background-remnants.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/curved-rail-wood/curved-rail-vertical-right-top-stone-path-background.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/curved-rail-wood/curved-rail-vertical-right-top-stone-path-remnants.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/curved-rail-wood/curved-rail-vertical-right-top-stone-path.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/curved-rail-wood/curved-rail-vertical-right-top-ties-remnants.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/curved-rail-wood/curved-rail-vertical-right-top-ties.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/curved-rail-wood/hr-curved-rail-horizontal-left-bottom-backplates-remnants.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/curved-rail-wood/hr-curved-rail-horizontal-left-bottom-backplates.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/curved-rail-wood/hr-curved-rail-horizontal-left-bottom-metals-remnants.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/curved-rail-wood/hr-curved-rail-horizontal-left-bottom-metals.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/curved-rail-wood/hr-curved-rail-horizontal-left-bottom-stone-path-background-remnants.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/curved-rail-wood/hr-curved-rail-horizontal-left-bottom-stone-path-background.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/curved-rail-wood/hr-curved-rail-horizontal-left-bottom-stone-path-remnants.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/curved-rail-wood/hr-curved-rail-horizontal-left-bottom-stone-path.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/curved-rail-wood/hr-curved-rail-horizontal-left-bottom-ties-remnants.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/curved-rail-wood/hr-curved-rail-horizontal-left-bottom-ties.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/curved-rail-wood/hr-curved-rail-horizontal-left-top-backplates-remnants.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/curved-rail-wood/hr-curved-rail-horizontal-left-top-backplates.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/curved-rail-wood/hr-curved-rail-horizontal-left-top-metals-remnants.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/curved-rail-wood/hr-curved-rail-horizontal-left-top-metals.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/curved-rail-wood/hr-curved-rail-horizontal-left-top-stone-path-background-remnants.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/curved-rail-wood/hr-curved-rail-horizontal-left-top-stone-path-background.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/curved-rail-wood/hr-curved-rail-horizontal-left-top-stone-path-remnants.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/curved-rail-wood/hr-curved-rail-horizontal-left-top-stone-path.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/curved-rail-wood/hr-curved-rail-horizontal-left-top-ties-remnants.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/curved-rail-wood/hr-curved-rail-horizontal-left-top-ties.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/curved-rail-wood/hr-curved-rail-horizontal-right-bottom-backplates-remnants.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/curved-rail-wood/hr-curved-rail-horizontal-right-bottom-backplates.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/curved-rail-wood/hr-curved-rail-horizontal-right-bottom-metals-remnants.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/curved-rail-wood/hr-curved-rail-horizontal-right-bottom-metals.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/curved-rail-wood/hr-curved-rail-horizontal-right-bottom-stone-path-background-remnants.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/curved-rail-wood/hr-curved-rail-horizontal-right-bottom-stone-path-background.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/curved-rail-wood/hr-curved-rail-horizontal-right-bottom-stone-path-remnants.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/curved-rail-wood/hr-curved-rail-horizontal-right-bottom-stone-path.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/curved-rail-wood/hr-curved-rail-horizontal-right-bottom-ties-remnants.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/curved-rail-wood/hr-curved-rail-horizontal-right-bottom-ties.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/curved-rail-wood/hr-curved-rail-horizontal-right-top-backplates-remnants.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/curved-rail-wood/hr-curved-rail-horizontal-right-top-backplates.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/curved-rail-wood/hr-curved-rail-horizontal-right-top-metals-remnants.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/curved-rail-wood/hr-curved-rail-horizontal-right-top-metals.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/curved-rail-wood/hr-curved-rail-horizontal-right-top-stone-path-background-remnants.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/curved-rail-wood/hr-curved-rail-horizontal-right-top-stone-path-background.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/curved-rail-wood/hr-curved-rail-horizontal-right-top-stone-path-remnants.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/curved-rail-wood/hr-curved-rail-horizontal-right-top-stone-path.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/curved-rail-wood/hr-curved-rail-horizontal-right-top-ties-remnants.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/curved-rail-wood/hr-curved-rail-horizontal-right-top-ties.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/curved-rail-wood/hr-curved-rail-vertical-left-bottom-backplates-remnants.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/curved-rail-wood/hr-curved-rail-vertical-left-bottom-backplates.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/curved-rail-wood/hr-curved-rail-vertical-left-bottom-metals-remnants.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/curved-rail-wood/hr-curved-rail-vertical-left-bottom-metals.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/curved-rail-wood/hr-curved-rail-vertical-left-bottom-stone-path-background-remnants.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/curved-rail-wood/hr-curved-rail-vertical-left-bottom-stone-path-background.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/curved-rail-wood/hr-curved-rail-vertical-left-bottom-stone-path-remnants.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/curved-rail-wood/hr-curved-rail-vertical-left-bottom-stone-path.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/curved-rail-wood/hr-curved-rail-vertical-left-bottom-ties-remnants.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/curved-rail-wood/hr-curved-rail-vertical-left-bottom-ties.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/curved-rail-wood/hr-curved-rail-vertical-left-top-backplates-remnants.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/curved-rail-wood/hr-curved-rail-vertical-left-top-backplates.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/curved-rail-wood/hr-curved-rail-vertical-left-top-metals-remnants.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/curved-rail-wood/hr-curved-rail-vertical-left-top-metals.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/curved-rail-wood/hr-curved-rail-vertical-left-top-stone-path-background-remnants.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/curved-rail-wood/hr-curved-rail-vertical-left-top-stone-path-background.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/curved-rail-wood/hr-curved-rail-vertical-left-top-stone-path-remnants.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/curved-rail-wood/hr-curved-rail-vertical-left-top-stone-path.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/curved-rail-wood/hr-curved-rail-vertical-left-top-ties-remnants.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/curved-rail-wood/hr-curved-rail-vertical-left-top-ties.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/curved-rail-wood/hr-curved-rail-vertical-right-bottom-backplates-remnants.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/curved-rail-wood/hr-curved-rail-vertical-right-bottom-backplates.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/curved-rail-wood/hr-curved-rail-vertical-right-bottom-metals-remnants.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/curved-rail-wood/hr-curved-rail-vertical-right-bottom-metals.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/curved-rail-wood/hr-curved-rail-vertical-right-bottom-stone-path-background-remnants.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/curved-rail-wood/hr-curved-rail-vertical-right-bottom-stone-path-background.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/curved-rail-wood/hr-curved-rail-vertical-right-bottom-stone-path-remnants.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/curved-rail-wood/hr-curved-rail-vertical-right-bottom-stone-path.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/curved-rail-wood/hr-curved-rail-vertical-right-bottom-ties-remnants.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/curved-rail-wood/hr-curved-rail-vertical-right-bottom-ties.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/curved-rail-wood/hr-curved-rail-vertical-right-top-backplates-remnants.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/curved-rail-wood/hr-curved-rail-vertical-right-top-backplates.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/curved-rail-wood/hr-curved-rail-vertical-right-top-metals-remnants.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/curved-rail-wood/hr-curved-rail-vertical-right-top-metals.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/curved-rail-wood/hr-curved-rail-vertical-right-top-stone-path-background-remnants.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/curved-rail-wood/hr-curved-rail-vertical-right-top-stone-path-background.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/curved-rail-wood/hr-curved-rail-vertical-right-top-stone-path-remnants.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/curved-rail-wood/hr-curved-rail-vertical-right-top-stone-path.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/curved-rail-wood/hr-curved-rail-vertical-right-top-ties-remnants.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/curved-rail-wood/hr-curved-rail-vertical-right-top-ties.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/straight-rail-wood/hr-straight-rail-diagonal-left-bottom-backplates-remnants.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/straight-rail-wood/hr-straight-rail-diagonal-left-bottom-backplates.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/straight-rail-wood/hr-straight-rail-diagonal-left-bottom-metals-remnants.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/straight-rail-wood/hr-straight-rail-diagonal-left-bottom-metals.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/straight-rail-wood/hr-straight-rail-diagonal-left-bottom-stone-path-background-remnants.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/straight-rail-wood/hr-straight-rail-diagonal-left-bottom-stone-path-background.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/straight-rail-wood/hr-straight-rail-diagonal-left-bottom-stone-path-remnants.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/straight-rail-wood/hr-straight-rail-diagonal-left-bottom-stone-path.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/straight-rail-wood/hr-straight-rail-diagonal-left-bottom-ties-remnants.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/straight-rail-wood/hr-straight-rail-diagonal-left-bottom-ties.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/straight-rail-wood/hr-straight-rail-diagonal-left-top-backplates-remnants.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/straight-rail-wood/hr-straight-rail-diagonal-left-top-backplates.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/straight-rail-wood/hr-straight-rail-diagonal-left-top-metals-remnants.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/straight-rail-wood/hr-straight-rail-diagonal-left-top-metals.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/straight-rail-wood/hr-straight-rail-diagonal-left-top-stone-path-background-remnants.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/straight-rail-wood/hr-straight-rail-diagonal-left-top-stone-path-background.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/straight-rail-wood/hr-straight-rail-diagonal-left-top-stone-path-remnants.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/straight-rail-wood/hr-straight-rail-diagonal-left-top-stone-path.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/straight-rail-wood/hr-straight-rail-diagonal-left-top-ties-remnants.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/straight-rail-wood/hr-straight-rail-diagonal-left-top-ties.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/straight-rail-wood/hr-straight-rail-diagonal-right-bottom-backplates-remnants.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/straight-rail-wood/hr-straight-rail-diagonal-right-bottom-backplates.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/straight-rail-wood/hr-straight-rail-diagonal-right-bottom-metals-remnants.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/straight-rail-wood/hr-straight-rail-diagonal-right-bottom-metals.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/straight-rail-wood/hr-straight-rail-diagonal-right-bottom-stone-path-background-remnants.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/straight-rail-wood/hr-straight-rail-diagonal-right-bottom-stone-path-background.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/straight-rail-wood/hr-straight-rail-diagonal-right-bottom-stone-path-remnants.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/straight-rail-wood/hr-straight-rail-diagonal-right-bottom-stone-path.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/straight-rail-wood/hr-straight-rail-diagonal-right-bottom-ties-remnants.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/straight-rail-wood/hr-straight-rail-diagonal-right-bottom-ties.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/straight-rail-wood/hr-straight-rail-diagonal-right-top-backplates-remnants.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/straight-rail-wood/hr-straight-rail-diagonal-right-top-backplates.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/straight-rail-wood/hr-straight-rail-diagonal-right-top-metals-remnants.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/straight-rail-wood/hr-straight-rail-diagonal-right-top-metals.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/straight-rail-wood/hr-straight-rail-diagonal-right-top-stone-path-background-remnants.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/straight-rail-wood/hr-straight-rail-diagonal-right-top-stone-path-background.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/straight-rail-wood/hr-straight-rail-diagonal-right-top-stone-path-remnants.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/straight-rail-wood/hr-straight-rail-diagonal-right-top-stone-path.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/straight-rail-wood/hr-straight-rail-diagonal-right-top-ties-remnants.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/straight-rail-wood/hr-straight-rail-diagonal-right-top-ties.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/straight-rail-wood/hr-straight-rail-horizontal-backplates-remnants.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/straight-rail-wood/hr-straight-rail-horizontal-backplates.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/straight-rail-wood/hr-straight-rail-horizontal-metals-remnants.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/straight-rail-wood/hr-straight-rail-horizontal-metals.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/straight-rail-wood/hr-straight-rail-horizontal-stone-path-background-remnants.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/straight-rail-wood/hr-straight-rail-horizontal-stone-path-background.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/straight-rail-wood/hr-straight-rail-horizontal-stone-path-remnants.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/straight-rail-wood/hr-straight-rail-horizontal-stone-path.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/straight-rail-wood/hr-straight-rail-horizontal-ties-remnants.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/straight-rail-wood/hr-straight-rail-horizontal-ties.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/straight-rail-wood/hr-straight-rail-vertical-backplates-remnants.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/straight-rail-wood/hr-straight-rail-vertical-backplates.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/straight-rail-wood/hr-straight-rail-vertical-metals-remnants.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/straight-rail-wood/hr-straight-rail-vertical-metals.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/straight-rail-wood/hr-straight-rail-vertical-stone-path-background-remnants.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/straight-rail-wood/hr-straight-rail-vertical-stone-path-background.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/straight-rail-wood/hr-straight-rail-vertical-stone-path-remnants.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/straight-rail-wood/hr-straight-rail-vertical-stone-path.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/straight-rail-wood/hr-straight-rail-vertical-ties-remnants.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/straight-rail-wood/hr-straight-rail-vertical-ties.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/straight-rail-wood/straight-rail-diagonal-left-bottom-backplates-remnants.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/straight-rail-wood/straight-rail-diagonal-left-bottom-backplates.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/straight-rail-wood/straight-rail-diagonal-left-bottom-metals-remnants.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/straight-rail-wood/straight-rail-diagonal-left-bottom-metals.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/straight-rail-wood/straight-rail-diagonal-left-bottom-stone-path-background-remnants.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/straight-rail-wood/straight-rail-diagonal-left-bottom-stone-path-background.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/straight-rail-wood/straight-rail-diagonal-left-bottom-stone-path-remnants.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/straight-rail-wood/straight-rail-diagonal-left-bottom-stone-path.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/straight-rail-wood/straight-rail-diagonal-left-bottom-ties-remnants.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/straight-rail-wood/straight-rail-diagonal-left-bottom-ties.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/straight-rail-wood/straight-rail-diagonal-left-top-backplates-remnants.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/straight-rail-wood/straight-rail-diagonal-left-top-backplates.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/straight-rail-wood/straight-rail-diagonal-left-top-metals-remnants.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/straight-rail-wood/straight-rail-diagonal-left-top-metals.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/straight-rail-wood/straight-rail-diagonal-left-top-stone-path-background-remnants.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/straight-rail-wood/straight-rail-diagonal-left-top-stone-path-background.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/straight-rail-wood/straight-rail-diagonal-left-top-stone-path-remnants.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/straight-rail-wood/straight-rail-diagonal-left-top-stone-path.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/straight-rail-wood/straight-rail-diagonal-left-top-ties-remnants.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/straight-rail-wood/straight-rail-diagonal-left-top-ties.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/straight-rail-wood/straight-rail-diagonal-right-bottom-backplates-remnants.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/straight-rail-wood/straight-rail-diagonal-right-bottom-backplates.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/straight-rail-wood/straight-rail-diagonal-right-bottom-metals-remnants.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/straight-rail-wood/straight-rail-diagonal-right-bottom-metals.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/straight-rail-wood/straight-rail-diagonal-right-bottom-stone-path-background-remnants.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/straight-rail-wood/straight-rail-diagonal-right-bottom-stone-path-background.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/straight-rail-wood/straight-rail-diagonal-right-bottom-stone-path-remnants.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/straight-rail-wood/straight-rail-diagonal-right-bottom-stone-path.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/straight-rail-wood/straight-rail-diagonal-right-bottom-ties-remnants.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/straight-rail-wood/straight-rail-diagonal-right-bottom-ties.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/straight-rail-wood/straight-rail-diagonal-right-top-backplates-remnants.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/straight-rail-wood/straight-rail-diagonal-right-top-backplates.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/straight-rail-wood/straight-rail-diagonal-right-top-metals-remnants.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/straight-rail-wood/straight-rail-diagonal-right-top-metals.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/straight-rail-wood/straight-rail-diagonal-right-top-stone-path-background-remnants.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/straight-rail-wood/straight-rail-diagonal-right-top-stone-path-background.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/straight-rail-wood/straight-rail-diagonal-right-top-stone-path-remnants.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/straight-rail-wood/straight-rail-diagonal-right-top-stone-path.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/straight-rail-wood/straight-rail-diagonal-right-top-ties-remnants.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/straight-rail-wood/straight-rail-diagonal-right-top-ties.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/straight-rail-wood/straight-rail-horizontal-backplates-remnants.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/straight-rail-wood/straight-rail-horizontal-backplates.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/straight-rail-wood/straight-rail-horizontal-metals-remnants.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/straight-rail-wood/straight-rail-horizontal-metals.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/straight-rail-wood/straight-rail-horizontal-stone-path-background-remnants.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/straight-rail-wood/straight-rail-horizontal-stone-path-background.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/straight-rail-wood/straight-rail-horizontal-stone-path-remnants.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/straight-rail-wood/straight-rail-horizontal-stone-path.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/straight-rail-wood/straight-rail-horizontal-ties-remnants.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/straight-rail-wood/straight-rail-horizontal-ties.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/straight-rail-wood/straight-rail-vertical-backplates-remnants.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/straight-rail-wood/straight-rail-vertical-backplates.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/straight-rail-wood/straight-rail-vertical-metals-remnants.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/straight-rail-wood/straight-rail-vertical-metals.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/straight-rail-wood/straight-rail-vertical-stone-path-background-remnants.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/straight-rail-wood/straight-rail-vertical-stone-path-background.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/straight-rail-wood/straight-rail-vertical-stone-path-remnants.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/straight-rail-wood/straight-rail-vertical-stone-path.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/straight-rail-wood/straight-rail-vertical-ties-remnants.png",
"/tmp/xx/JunkTrain_0.0.8/graphics/straight-rail-wood/straight-rail-vertical-ties.png"
];

const hrz = "horizontal";
const vrt = "vertical";
const crv_hlb = hrz+"-left-bottom";
const crv_hlt = hrz+"-left-top";
const crv_hrb = hrz+"-right-bottom";
const crv_hrt = hrz+"-right-top";
const crv_vlb = vrt+"-left-bottom";
const crv_vlt = vrt+"-left-top";
const crv_vrb = vrt+"-right-bottom";
const crv_vrt = vrt+"-right-top";

const dia = "diagonal";
const dia_rb = dia + "-right-bottom";
const dia_rt = dia + "-right-top";
const dia_lb = dia + "-left-bottom";
const dia_lt = dia + "-left-top";

const remnant = "remnants";
const bed = "stone-path";
const bed_back = bed+"-background";
const ties = "ties";
const metals = "metals";
const backplates = "backplates";

const curve_prefix = "curved-rail";
const hr_curve_prefix = "hr-curved-rail";
const straight_prefix = "straight-rail";
const hr_straight_prefix = "hr-straight-rail";

const STR = 1;
const DIA = 2;
const CRV = 256;
const VER = 4;
const HOR = 8;
const LFT = 16;
const TOP = 32;
const RGT = 64;
const BOT = 128;

const WIDTH = 19;
const CENTER = 9;

const circle = [ { type: STR|VER        , pos = { x: 1, y: CENTER-1 } },
                 { type: CRV|VER|RGT|BOT, pos = { x: 1, y: CENTER_3 } },
                 { type: STR|DIA|RGT|BOT, pos = { x: 1, y: 3 } },


function sortNames() {
	var straights = [];
	var curves = [];
	rails.forEach( (rail)=>{
        	var parts = rail.split( "/" );
                var last = parts[parts.length-1];
                var base;
                if( last.startsWith( "hr-" ) )
                 	base  = last.susbstr( 3 );
                else base = last;
                if( last.contains( 'straight' ) )
                	straights.push(last);
                else curves.push( last );
                	
        } );
}

