function [OrderedNeighborsCell]=GetNeighbors(V,F)
%ORDEREDNEIGHBORS Summary of this function goes here
%   Detailed explanation goes here

OrderedNeighborsCell={};
nv=size(V,2);
nt=size(F,2);

IncidenceMatrix=sparse([1:nt,1:nt,1:nt],[F(1,:),F(2,:),F(3,:)],1,nt,nv);

for i=1:nv
    [triangles] = find(IncidenceMatrix(:,i));
    numNeigh=0;
    foundNeighbors=0;
    TargetNeighbors=size(triangles,1);
    k=0;
    start=1;
    while numNeigh<TargetNeighbors
		k=k+1; 
		j=triangles(k);
		if(start==1)
			if(F(1,j)==i)
				Neigh=[F(2,j),F(3,j)];
			elseif(F(2,j)==i) 
				Neigh=[F(3,j),F(1,j)];
			else
				Neigh=[F(1,j),F(2,j)];
			end
			foundNeighbors=1;
			start=0;
		else
				if(i==F(1,j))
					Neigh=[Neigh;F(2,j),F(3,j)];
					foundNeighbors=1;
				elseif(i==F(2,j))
					Neigh=[Neigh;F(3,j),F(1,j)];
					foundNeighbors=1;
				elseif(i==F(3,j))
					Neigh=[Neigh;F(1,j),F(2,j)];
					foundNeighbors=1;
				end
		end
		if(foundNeighbors==1)
			triangles(k)=[];
			numNeigh=numNeigh+1;
			foundNeighbors=0;   
			k=0;
		end
    end
    OrderedNeighborsCell{1,i}=Neigh;
end

