function parsePrefixForFieldsplit(SAWs_prefix) {

    var fieldsplit="0";

    if(SAWs_prefix.indexOf("fieldsplit_")!=-1) {//still contains at least 1 fieldsplit (assume max of 1 fieldsplit for now)

        var closest=SAWs_prefix.length;//the furthest a keyword could possibly be
        //find index of next keyword (pc, ksp, sub, smoothing, coarse)
        var keywords=["pc","ksp","sub","smoothing","coarse","redundant","mg"];
        var loc="";
        for(var i=0; i<keywords.length; i++) {
            loc=SAWs_prefix.indexOf(keywords[i]);
            if(loc < closest && loc != -1)
                closest=loc;
        }

        var theword = SAWs_prefix.substring(11,closest-1);//omit the first and last underscore

        var fieldsplitNumber = getFieldsplitWordIndex(theword);

        if(fieldsplitNumber == -1) {// new fieldsplit
            fieldsplitKeywords[fieldsplitKeywords.length] = theword;//record new keyword
            fieldsplitNumber = fieldsplitKeywords.length - 1;
            var writeLoc = sawsInfo.length;
            sawsInfo[writeLoc]      = new Object();
            sawsInfo[writeLoc].data = new Array();
            sawsInfo[writeLoc].id   = fieldsplit + fieldsplitNumber.toString();
        }
        fieldsplit = fieldsplit + fieldsplitNumber.toString();
    }

    return fieldsplit;

}

function parsePrefixForEndtag(SAWs_prefix, index) {

    var endtag="";
    while(SAWs_prefix!="") {//parse the entire prefix
        var indexFirstUnderscore=SAWs_prefix.indexOf("_");
        var chunk=SAWs_prefix.substring(0,indexFirstUnderscore);//dont include the underscore

        if(chunk=="mg") {//mg_
            indexFirstUnderscore=SAWs_prefix.indexOf("_",3); //this will actually be the index of the second underscore now since mg prefix has underscore in itself
            chunk=SAWs_prefix.substring(0,indexFirstUnderscore);//updated chunk
        }

        if(chunk=="mg_levels") {//need to include yet another underscore
            indexFirstUnderscore=SAWs_prefix.indexOf("_",10); //this will actually be the index of the third underscore
            chunk=SAWs_prefix.substring(0,indexFirstUnderscore);//updated chunk
        }

        SAWs_prefix=SAWs_prefix.substring(indexFirstUnderscore+1, SAWs_prefix.length);//dont include the underscore

        if(chunk=="mg_coarse" && SAWs_prefix=="")//new mg coarse
            mgLevelLocation=endtag+"0";

        if(chunk=="ksp" || chunk=="sub" || chunk=="mg_coarse" || chunk=="redundant")
            endtag+="0";
        else if(chunk.substring(0,10)=="mg_levels_")
            endtag+=chunk.substring(10,11);//can only be 1 character long for now

        if(chunk.substring(0,10)=="mg_levels_" && SAWs_prefix=="")//new mg levels. it's okay to assume memory was already allocated b/c levels is after coarse
            sawsInfo[index].data[getSawsDataIndex(index, mgLevelLocation)].mg_levels=parseInt(chunk.substring(10,11))+1;
    }

    return endtag;
}