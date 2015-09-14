
function bang()
{
	var m = new JitterMatrix(3,"float32",10,10,11);

	post("planecount: "+m.planecount+"\n");
	post("type: "+m.type+"\n");
	post("dim: "+m.dim+"\n");

	m.setcell(0,0,"val",24,24,24,24);
	post("cell 0,0 = " + m.getcell(0,0) + "\n");
}

var charlie = new JitterMatrix();
charlie.type = "char";
charlie.planecount = 4;
charlie.dim = [100,100];
charlie.name = "charly";

function chomp()
{
	var i,j,width,height;

	// *way* more effiient to cache this here, instead of referencing a property each time
	width = charlie.dim[0];
	height = charlie.dim[1];

	for (i=0;i<height;i++) {
		for (j=0;j<width;j++) {
			//charlie.setcell(j,i,"val",j,i,j,i);
			charlie.setcell2d(j,i,j,i,j,i);
			//charlie.setcell(j,i,"plane",0,"val",i);
			//charlie.setplane2d(j,i,0,i);
		}
	}
	//post("charlie dims = " + charlie.dim + "\n");
	outlet(0,"jit_matrix",charlie.name);
}

function champ(x,y)
{
	charlie.setinfo(4,"char",x,y);
}

function chimp(x,y)
{
	//post("BEGIN cell 10,10 = " + charlie.getcell(10,10) + "\n");
	charlie.op("+",1,2,3,4);
	//post("END cell 10,10 = " + charlie.getcell(10,10) + "\n");
	outlet(0,"jit_matrix",charlie.name);
}

function jit_matrix(inname)
{
	var input = new JitterMatrix(inname);
	//var input = new JitterMatrix(); 
	//input.name = inname;	
	//charlie.setinfo(input.planecount, input.type, input.dim);
	charlie.setinfo(input); // gets info from matrix 
	charlie.clear();
	
//	post("cell = " + input.getcell(4, 4) + "\n");


	var i,j,width,height, counter;
	
	var cell;

	// *way* more effiient to cache this here, instead of referencing a property each time
	width = charlie.dim[0];
	height = charlie.dim[1];


	for (i=0;i<height;i++) {
		for (j=0;j<width;j++) {
			cell = input.getcell(j, i);
			if(cell[1] > 0){
				charlie.setcell2d(j,i,255,255,0,0);
			}
		}
	}

	//charlie.op("*",inname,inname);
	//charlie.op("*",input,input);
	outlet(0,"jit_matrix",charlie.name);	
}

function copy(inname)
{ 
	charlie.adapt = 0;
	charlie.interp = 0;
//	charlie.frommatrix(inname);
	var input = new JitterMatrix(inname);
	charlie.frommatrix(input);

	outlet(0,"jit_matrix",charlie.name);	
}

function copyinterp(inname)
{ 
	charlie.adapt = 0;
	charlie.interp = 1;
	charlie.frommatrix(inname);
	outlet(0,"jit_matrix",charlie.name);	
}

var brcosa = new JitterObject("jit.brcosa");
var sobel = new JitterObject("jit.sobel");
var scalebias = new JitterObject("jit.scalebias");
var freddy = new JitterMatrix();

// matrix names and matrix objects can be used interchangeably
function azreal(inname)
{ 
	charlie.setinfo(inname);
	freddy.setinfo(inname);
	brcosa.contrast = 2.;
	brcosa.matrixcalc(inname,freddy);
	sobel.thresh = 0.2; 
	sobel.mode = 1; 
	sobel.matrixcalc(freddy,charlie); 
	charlie.op("~",charlie); 
	scalebias.ascale = 0;
	scalebias.rscale = 0.8;
	scalebias.gscale = 1.;
	scalebias.bscale = 0.6;
	scalebias.rbias = -0.5;
	scalebias.gbias = -0.5;
	scalebias.bbias = -0.5;
	scalebias.mode = 1;
	scalebias.matrixcalc(charlie,charlie);
	charlie.op("*",inname); 
	outlet(0,"jit_matrix",charlie.name);	
}

function fading(inname)
{ 
	var xfader = new JitterObject("jit.xfade");
	charlie.setinfo(inname); //adapt
	xfader.xfade = 0.97;
	xfader.matrixcalc([inname,charlie],charlie);
	outlet(0,"jit_matrix",charlie.name);	
}

function gargamel(inname)
{ 
	var input = new JitterMatrix(inname);
	var foo = new JitterObject("jit.rgb2luma");
	charlie.setinfo(1,input.type,input.dim);
	foo.matrixcalc(input,charlie);
	outlet(0,"jit_matrix",charlie.name);	
}

var output_a = new JitterMatrix();
var output_r = new JitterMatrix();
var output_g = new JitterMatrix();
var output_b = new JitterMatrix();

function redtester(inname)
{ 
	var input = new JitterMatrix(inname);
	var unpacker = new JitterObject("jit.unpack");

	output_a.setinfo(1,input.type,input.dim);
	output_r.setinfo(1,input.type,input.dim);
	output_g.setinfo(1,input.type,input.dim);
	output_b.setinfo(1,input.type,input.dim);
		
	// note array for outputs
	unpacker.matrixcalc(input,[output_a,output_r,output_g,output_b]);
	outlet(0,"jit_matrix",output_r.name);	
}


var mov = new JitterObject("jit.qt.movie");
mov.read("dozer.mov");

function movtester()
{ 
	charlie.setinfo(4,"char",mov.moviedim);
	mov.matrixcalc(charlie/*null*/,charlie);
	outlet(0,"jit_matrix",charlie.name);	
}
