#ifndef IMOPR_H
#define IMOPR_H

extern	int	imstat(const	float	*pix,
		       const	int	pdim,
		       const	int	nrej, 
		       		int	*nsky,
		       		float	*meen,
		       		float	*sigma);

extern	int	imextra(const	float	*pix,
			const	int	npx,
			const	int	npy,
			const	int	x0,
			const	int	y0,
			const	int	x1,
			const	int	y1,
				float	*tmp);

extern	int	clean_pix(	float	*pix,
			  const	int	npx,
			  const	int	npy,
			  const	int	*map);


extern int skydet(const float*, const int, const int, int*, float*, float*, float);

#endif
