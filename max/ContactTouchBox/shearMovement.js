
function jit_matrix(inname)
{
	var input = new JitterMatrix(inname);
	
	var posX = 1240;
	var posY = 960/2;
	
	var pix;
	
	var sumY = 0;
	var counterY = 0.;
	
	var sumX = 0;
	var counterX = 0.;
	
	for(var i = posY - 20; i < posY + 20; i++){
		pix = input.getcell(posX, i);
		if(pix[3] == 255){
			sumY += i;
			counterY++;
		}
	}

	outlet(0, "posY", posY - (sumY / counterY));

	posX = 1280 / 2;
	var posY = 840;
		
	for(i = posX - 40; i < posX + 40; i++){
		pix = input.getcell(i, posY);
//		outlet(0, "pix", i, pix);

		if(pix[3] == 255){
			sumX += i;
			counterX++;
		}
	}

	outlet(0, "posX", posX - (sumX / counterX));
}

