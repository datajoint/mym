% test serialize and deserialize commands

% array
fprintf('\nTesting Array...\n') ;
for ii=1:1000
    a = rand(3,3) ;
    b = mym('serialize {M}',a) ;
    bb = b{1} ;
    c = mym('deserialize',bb) ;
    if (sum(norm(a-c,2)))
        fprintf('array serialization-deserialization did not work\n') ;
        break ;
    end
    if (rem(ii,100) == 0)
        fprintf('Iteration # %d\n', ii) ;
    end
end


% struct
fprintf('\nTesting Struct...\n') ;
a = struct('x',0,'y',0) ;
for ii=1:1000
    a.x = rand(3,3) ;
    a.y = rand(3,3) ;
    b = mym('serialize {M}',a) ;
    bb = b{1} ;
    c = mym('deserialize',bb) ;
    if (sum(norm(a.x-c.x,2)+norm(a.y-c.y,2)))
        fprintf('struct serialization-deserialization did not work\n') ;
        break ;
    end
    if (rem(ii,100) == 0)
        fprintf('Iteration # %d\n', ii) ;
    end
end 

% cell
fprintf('\nTesting Cell...\n') ;
for ii=1:1000
    a = {rand(3,3), rand(2,2)} ;
    b = mym('serialize {M}',a) ;
    bb = b{1} ;
    c = mym('deserialize',bb) ;
    if (sum(norm(a{1}-c{1},2)+norm(a{2}-c{2},2)))
        fprintf('cell serialization-deserialization did not work\n') ;
        break ;
    end
    if (rem(ii,100) == 0)
        fprintf('Iteration # %d\n', ii) ;
    end
end 